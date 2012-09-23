/*! @file render_gl.cpp
 *  @author Kyle Weicht
 *  @date 9/13/12
 *  @copyright Copyright (c) 2012 Kyle Weicht. All rights reserved.
 */
#include "render.h"

#ifdef __APPLE__
    #include "TargetConditionals.h"
    #if TARGET_OS_IPHONE
        #include <OpenGLES/ES2/gl.h>
    #elif TARGET_OS_MAC
        #include <OpenGL/gl3.h>
        #define GL_COMPRESSED_RGB_S3TC_DXT1_EXT 0x83F0
        #define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT 0x83F1
        #define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT 0x83F2
        #define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT 0x83F3
    #endif
    // TODO: flushing the buffer requires Obj-C in OS X. This is a hack to
    // include an Obj-C function
    extern "C" void _osx_flush_buffer(void* window);
#elif defined(_WIN32)
    #include <gl/glew.h>
    #include <gl/wglew.h>
    #include <gl/glcorearb.h>
    #pragma warning(disable:4127) /* Conditional expression is constant */
    #define snprintf _snprintf
#endif

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include "assert.h"
#include "stb_image.h"
#include "application.h"
#include "vec_math.h"
#include "geometry.h"
#include "render_gl_helper.h"

#include "renderer.h"
#include "renderer_deferred.h"
#include "renderer_forward.h"

#define CheckGLError()                  \
    do {                                \
        GLenum _glError = glGetError(); \
        if(_glError != GL_NO_ERROR) {   \
            debug_output("OpenGL Error: %d\n", _glError);\
        }                               \
        assert(_glError == GL_NO_ERROR);\
    } while(__LINE__ == 0)

#define MAKEFOURCC(ch0, ch1, ch2, ch3)                              \
                ((uint32_t)(uint8_t)(ch0) | ((uint32_t)(uint8_t)(ch1) << 8) |   \
                ((uint32_t)(uint8_t)(ch2) << 16) | ((uint32_t)(uint8_t)(ch3) << 24 ))
#define FOURCC_DXT1	MAKEFOURCC('D', 'X', 'T', '1')
#define FOURCC_DXT3	MAKEFOURCC('D', 'X', 'T', '3')
#define FOURCC_DXT5	MAKEFOURCC('D', 'X', 'T', '5')

namespace {

enum UniformBufferType {
    kWorldTransformBuffer,
    kViewProjTransformBuffer,
    kLightBuffer,

    kNUM_UNIFORM_BUFFERS
};
enum VertexShaderType {
    kVSPos,

    kNUM_VERTEX_SHADERS
};
enum FragmentShaderType {
    kFSColor,

    kNUM_FRAGMENT_SHADERS
};
enum ProgramType {
    k3DProgram,
    k2DProgram,
    kDeferredProgram,
    kDeferredLightProgram,
    kDebugProgram,

    kNUM_PROGRAMS
};


enum { kMAX_MESHES = 1024, kMAX_TEXTURES = 64, kMAX_RENDER_COMMANDS = 1024*8 };

static const VertexDescription kVertexDescriptions[kNUM_VERTEX_TYPES][8] =
{
    { // kVtxPosNormTex
        { 0, 3 },
        { 1, 3 },
        { 4, 2 },
        { 0, 0 },
    },
    { // kVtxPosTex
        { 0, 3 },
        { 1, 2 },
        { 0, 0 },
    },
    { // kVtxPosNormTanBitanTex
        { 0, 3 },
        { 1, 3 },
        { 2, 3 },
        { 3, 3 },
        { 4, 2 },
        { 0, 0 },
    },
};
static const size_t kVertexSizes[kNUM_VERTEX_TYPES] =
{
    sizeof(VtxPosNormTex),
    sizeof(VtxPosTex),
    sizeof(VtxPosNormTanBitanTex)
};

}

class RenderGL : public Render {
public:

RenderGL()
    : _window(NULL)
    , _num_meshes(0)
    , _num_textures(0)
    , _num_2d_render_commands(0)
    , _num_3d_render_commands(0)
    , _3d_view(float4x4identity)
    , _2d_view(float4x4identity)
    , _width(128)
    , _height(128)
    , _deferred(1)
    , _debug(0)
    , _num_renderables(0)
{
    _light_buffer.num_lights = 0;
}
~RenderGL() {
}

void* window(void) { return _window; }

void initialize(void* window) {
#ifdef _WIN32
    HWND hWnd = (HWND)window;
    HDC hDC = GetDC(hWnd);

    PIXELFORMATDESCRIPTOR   pfd = {0};
    pfd.nSize       = sizeof(pfd);
    pfd.nVersion    = 1;
    pfd.dwFlags     = PFD_DOUBLEBUFFER | PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW;
    pfd.iPixelType  = PFD_TYPE_RGBA;
    pfd.cColorBits  = 24;
    pfd.cDepthBits  = 24;
    pfd.iLayerType  = PFD_MAIN_PLANE;

    int pixel_format = ChoosePixelFormat(hDC, &pfd);
    assert(pixel_format);

    int result = SetPixelFormat(hDC, pixel_format, &pfd);
    assert(result);

    // Create a dummy OpenGL 1.1 context to use for Glew initialization
    HGLRC first_GLRC = wglCreateContext(hDC);
    assert(first_GLRC);
    wglMakeCurrent(hDC, first_GLRC);
    CheckGLError();

    // Glew
    GLenum error = glewInit();
    CheckGLError();
    if(error != GLEW_OK) {
        debug_output("Glew Error: %s\n", glewGetErrorString(error));
    }
    assert(error == GLEW_OK);
    assert(wglewIsSupported("WGL_ARB_extensions_string") == 1);
    assert(wglewIsSupported("WGL_ARB_create_context") == 1);

    // Now create the real, OpenGL 3.2 context
    GLint attributes[] = {
        WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
        WGL_CONTEXT_MINOR_VERSION_ARB, 2,
        0,
    };
    HGLRC new_GLRC = wglCreateContextAttribsARB(hDC, 0, attributes);
    assert(new_GLRC);
    CheckGLError();

    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(first_GLRC);
    wglMakeCurrent(hDC, new_GLRC);
    CheckGLError();
    _dc = hDC;

    // Disable vsync
    assert(wglewIsSupported("WGL_EXT_swap_control") == 1);
    wglSwapIntervalEXT(0);
    CheckGLError();
#endif
    _window = window;
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClearDepth(1.0f);
    _load_shaders();
    _create_base_meshes();
    _clear(); // Clear once so the first present isn't garbage

    glFrontFace(GL_CW);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    _create_framebuffer();

    _forward_renderer.init();
    _deferred_renderer.init();
    _deferred_renderer.set_sphere_mesh(_meshes[_sphere_mesh]);
    _deferred_renderer.set_fullscreen_mesh(_meshes[_fullscreen_quad_mesh]);
}
void shutdown(void) {
    _forward_renderer.shutdown();
    _deferred_renderer.shutdown();
}
void render(void) {
    _present();
    _clear();

    glBindFramebuffer(GL_FRAMEBUFFER, _frame_buffer);
    _clear();

    float4x4 view = _3d_view;
    float4x4 proj = _perspective_projection;

    if(_deferred)
        _deferred_renderer.render(view, proj, _frame_buffer,
                                  _renderables, _num_renderables,
                                  _light_buffer.lights, _light_buffer.num_lights);
    else
        _forward_renderer.render(view, proj, _frame_buffer,
                                 _renderables, _num_renderables,
                                 _light_buffer.lights, _light_buffer.num_lights);

    // Render the scene from the render target
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    if(_deferred && _debug) {
        glDisable(GL_DEPTH_TEST);
        const Mesh& mesh = _meshes[_fullscreen_quad_mesh];
        float4x4 translate = float4x4Scale(0.5f, 0.5f, 0.5f);
        translate.r3.x = -0.5f;
        translate.r3.y = -0.5f;
        glUseProgram(_2d_program);
        glBindVertexArray(mesh.vao);
        _validate_program(_2d_program);
        glUniformMatrix4fv(_2d_viewproj_uniform, 1, GL_FALSE, (float*)&float4x4identity);
        for(int ii=0;ii<4;++ii) {
            if(ii == 1)
                translate.r3.y = 0.5f;
            if(ii == 2)
                translate.r3.x = 0.5f;
            if(ii == 3)
                translate.r3.y = -0.5f;
            glBindTexture(GL_TEXTURE_2D, _deferred_renderer.gbuffer_tex(ii));
            glUniformMatrix4fv(_2d_world_uniform, 1, GL_FALSE, (float*)&translate);
            glDrawElements(GL_TRIANGLES, (GLsizei)mesh.index_count, mesh.index_format, NULL);
        }
        glEnable(GL_DEPTH_TEST);
    } else {
        _render_fullscreen(_color_texture);
    }

    _num_3d_render_commands = _num_2d_render_commands = 0;
    _num_renderables = 0;
    _light_buffer.num_lights = 0;
}
void resize(int width, int height) {
    _width = width;
    _height = height;
    glViewport(0, 0, width, height);
    _orthographic_projection = float4x4OrthographicOffCenterLH(0, (float)width, (float)height, 0, 0.0f, 1.0f);
    _perspective_projection = float4x4PerspectiveFovLH(DegToRad(50.0f), width/(float)height, 1.0f, 10000.0f);

    // Resize render targets
    _resize_framebuffer();
    _forward_renderer.resize(width, height);
    _deferred_renderer.resize(width, height);
}
void _render_fullscreen(GLuint texture) {
    glUseProgram(_fullscreen_program);
    glBindTexture(GL_TEXTURE_2D, texture);
    glBindVertexArray(_meshes[_fullscreen_quad_mesh].vao);
    _validate_program(_fullscreen_program);
    glDrawElements(GL_TRIANGLES, (GLsizei)_meshes[_fullscreen_quad_mesh].index_count, _meshes[_fullscreen_quad_mesh].index_format, NULL);
}

MeshID create_mesh(uint32_t vertex_count, VertexType vertex_type,
                   uint32_t index_count, size_t index_size,
                   const void* vertices, const void* indices) {
    MeshID id = _num_meshes++;

    if(vertex_type == kVtxPosNormTex) {
        VtxPosNormTanBitanTex* new_vertices = _calculate_tangets((VtxPosNormTex*)vertices, vertex_count, indices, index_size, index_count);
        vertex_type = kVtxPosNormTanBitanTex;
        _meshes[id] = _create_mesh(vertex_count, kVertexSizes[vertex_type],
                                   index_count, index_size,
                                   new_vertices, indices,
                                   kVertexDescriptions[vertex_type]);
        delete [] new_vertices;
    } else {
        _meshes[id] = _create_mesh(vertex_count, kVertexSizes[vertex_type],
                                   index_count, index_size,
                                   vertices, indices,
                                   kVertexDescriptions[vertex_type]);
    }
    return id;
}

MeshID cube_mesh(void) { return _cube_mesh; }
MeshID quad_mesh(void) { return _quad_mesh; }
MeshID sphere_mesh(void) { return _sphere_mesh; }
void toggle_debug_graphics(void) { _debug = !_debug; }
void toggle_deferred(void) {
    _deferred = !_deferred;
    if(_deferred)
        debug_output("Deferred\n");
    else
        debug_output("Forward\n");
}

void set_3d_view_matrix(const float4x4& view) {
    _3d_view = view;
}
void set_2d_view_matrix(const float4x4& view) {
    _2d_view = view;
}
void draw_3d(MeshID mesh, TextureID texture, TextureID normal_texture, const float4x4& transform) {
    const Mesh& m = _meshes[mesh];
    Renderable& r = _renderables[_num_renderables];
    r.texture = _textures[texture];
    r.normal_texture = _textures[normal_texture];
    r.vao = m.vao;
    r.index_count = m.index_count;
    r.index_format = m.index_format;
    r.transform = transform;

    _num_renderables++;
}
void draw_2d(MeshID, TextureID, const float4x4&) {
}

void draw_light(const Light& light) {
    int index = _light_buffer.num_lights++;
    _light_buffer.lights[index] = light;
}
TextureID load_texture(const char* filename) {
    int width, height, components;
    GLenum format;
    { // Check to see if this is a DXT texture
        FILE* file = fopen(filename, "rb");
        assert(file);
        char filecode[4];
        fread(filecode, 1, 4, file);
        if(strncmp(filecode, "DDS ", 4) == 0) {
            fclose(file);
            return _load_dxt_texture(filename);
        }
        fclose(file);
    }
    void* tex_data = stbi_load(filename, &width, &height, &components, 0);

    GLuint texture;
    glGenTextures(1, &texture);
    CheckGLError();
    glBindTexture(GL_TEXTURE_2D, texture);
    CheckGLError();

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    switch(components)
    {
    case 4:
        format = GL_RGBA;
        components = GL_RGBA;
        break;
    case 3:
        format = GL_RGB;
        components = GL_RGB;
        break;
    default: assert(0);
    }

    glTexImage2D(GL_TEXTURE_2D, 0, components, width, height, 0, format, GL_UNSIGNED_BYTE, tex_data);
    CheckGLError();
    glGenerateMipmap(GL_TEXTURE_2D);
    CheckGLError();
    glBindTexture(GL_TEXTURE_2D, 0);
    CheckGLError();

    stbi_image_free(tex_data);

    _textures[_num_textures] = texture;
    return _num_textures++;
}
TextureID _load_dxt_texture(const char* filename) {
    FILE* file = fopen(filename, "rb");
    assert(file);
    char filecode[4];
    fread(filecode, 1, 4, file);
    if(strncmp(filecode, "DDS ", 4) != 0) {
        fclose(file);
        return -1;
    }

    // Read the DXT header
    uint8_t header[124] = {0};
    fread(header, 124, 1, file);

    uint32_t height         = *(uint32_t*)&(header[8 ]);
    uint32_t width          = *(uint32_t*)&(header[12]);
    uint32_t linearSize     = *(uint32_t*)&(header[16]);
    uint32_t mipMapCount    = *(uint32_t*)&(header[24]);
    uint32_t fourCC         = *(uint32_t*)&(header[80]);

    uint8_t* buffer;
    uint32_t bufsize;
    /* how big is it going to be including all mipmaps? */
    bufsize = mipMapCount > 1 ? linearSize * 2 : linearSize;
    buffer = (uint8_t*)malloc(bufsize * sizeof(uint8_t));
    fread(buffer, 1, bufsize, file);
    /* close the file pointer */
    fclose(file);

    //uint32_t components  = (fourCC == FOURCC_DXT1) ? 3 : 4;
    uint32_t format;
    switch(fourCC)
    {
    case FOURCC_DXT1:
        format = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
        break;
    case FOURCC_DXT3:
        format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
        break;
    case FOURCC_DXT5:
        format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
        break;
    default:
        free(buffer);
        return -1;
    }

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    uint32_t blockSize = (format == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT) ? 8 : 16;
    uint32_t offset = 0;
 
    /* load the mipmaps */
    for (uint32_t level = 0; level < mipMapCount && (width || height); ++level)
    {
        uint32_t size = ((width+3)/4)*((height+3)/4)*blockSize;
        glCompressedTexImage2D(GL_TEXTURE_2D, level, format, width, height, 
            0, size, buffer + offset);
     
        offset += size;
        width  /= 2;
        height /= 2;
    }
    free(buffer);
    glBindTexture(GL_TEXTURE_2D, 0);

    _textures[_num_textures] = texture;
    return _num_textures++;
}

MeshID load_mesh(const char* filename) {
    uint32_t nVertexStride;
    uint32_t nVertexCount;
    uint32_t nIndexSize;
    uint32_t nIndexCount;
    char* pData;

    const char* ext = (filename + strlen(filename))-3;
    if(strncmp(ext, "obj", 4) == 0) {
        return _load_obj(filename);
    }

    FILE* pFile = fopen( filename, "rb" );
    fread( &nVertexStride, sizeof( nVertexStride ), 1, pFile );
    fread( &nVertexCount, sizeof( nVertexCount ), 1, pFile );
    fread( &nIndexSize, sizeof( nIndexSize ), 1, pFile );
    if(nIndexSize > 8) // Convert from bits to bytes
        nIndexSize /= 8;
    fread( &nIndexCount, sizeof( nIndexCount ), 1, pFile );

    pData   = new char[ (nVertexStride * nVertexCount) + (nIndexCount * nIndexSize) ];

    fread( pData, nVertexStride, nVertexCount, pFile );
    fread( pData + (nVertexStride * nVertexCount), nIndexSize, nIndexCount, pFile );
    fclose( pFile );

    void* indices = pData + (nVertexStride * nVertexCount);

    VtxPosNormTanBitanTex* new_vertices = _calculate_tangets((VtxPosNormTex*)pData, nVertexCount, indices, (size_t)nIndexSize, nIndexCount);

    MeshID mesh = create_mesh(nVertexCount, kVtxPosNormTanBitanTex, nIndexCount, nIndexSize, new_vertices, indices);
    delete [] pData;
    delete [] new_vertices;

    return mesh;
}
struct int3 {
    int p;
    int t;
    int n;
};
MeshID _load_obj(const char* filename) {
    
    std::vector<float3> positions;
    std::vector<float3> normals;
    std::vector<float2> texcoords;

    std::vector<int3>   indicies;
    
    FILE* file = fopen(filename, "rt");
    while(1) {
        char line_header[128];
        int res = fscanf(file, "%s", line_header);
        if(res == EOF)
            break;

        if(strcmp(line_header, "v") == 0) {
            float3 v;
            fscanf(file, "%f %f %f\n", &v.x, &v.y, &v.z);
            positions.push_back(v);
        } else if(strcmp(line_header, "vt") == 0) {
            float2 t;
            fscanf(file, "%f %f\n", &t.x, &t.y);
            texcoords.push_back(t);
        } else if(strcmp(line_header, "vn") == 0) {
            float3 n;
            fscanf(file, "%f %f %f\n", &n.x, &n.y, &n.z);
            normals.push_back(n);
        } else if(strcmp(line_header, "f") == 0) {
            int3 triangle[4];
            int matches = fscanf(file, "%d/%d/%d %d/%d/%d %d/%d/%d %d/%d/%d\n",
                                        &triangle[0].p, &triangle[0].t,&triangle[0].n,
                                        &triangle[1].p, &triangle[1].t,&triangle[1].n,
                                        &triangle[2].p, &triangle[2].t,&triangle[2].n,
                                        &triangle[3].p, &triangle[3].t,&triangle[3].n);
            if(matches != 9 && matches != 12) {
                debug_output("Can't load this OBJ\n");
                return -1;
            }
            indicies.push_back(triangle[0]);
            indicies.push_back(triangle[1]);
            indicies.push_back(triangle[2]);
            if(matches == 12) {
                indicies.push_back(triangle[0]);
                indicies.push_back(triangle[2]); 
                indicies.push_back(triangle[3]);
            }
        } else {
            // A comment?
            char buffer[1024];
            fgets(buffer, sizeof(buffer), file);
        }
    }
    fclose(file);

    VtxPosNormTex* vertices = new VtxPosNormTex[indicies.size()];
    for(int ii=0; ii<(int)indicies.size(); ++ii) {
        int pos_index = indicies[ii].p-1;
        int tex_index = indicies[ii].t-1;
        int norm_index = indicies[ii].n-1;
        VtxPosNormTex& vertex = vertices[ii];
        vertex.pos = positions[pos_index];
        vertex.tex = texcoords[tex_index];
        vertex.norm = normals[norm_index];
    }
    uint32_t* i = new uint32_t[indicies.size()];
    for(int ii=0;ii<(int)indicies.size();++ii)
        i[ii] = ii;

    int vertex_count = (int)indicies.size();
    int index_count = vertex_count;

    VtxPosNormTanBitanTex* new_vertices = _calculate_tangets(vertices, vertex_count, i, sizeof(uint32_t), index_count);

    MeshID mesh = create_mesh(vertex_count, kVtxPosNormTanBitanTex, index_count, sizeof(uint32_t), new_vertices, i);
    delete [] new_vertices;
    delete [] vertices;
    delete [] i;

    return mesh;
}
VtxPosNormTanBitanTex* _calculate_tangets(const VtxPosNormTex* vertices, int num_vertices, const void* indices, size_t index_size, int num_indices) {
    VtxPosNormTanBitanTex* new_vertices = new VtxPosNormTanBitanTex[num_vertices];
    for(int ii=0;ii<num_vertices;++ii) {
        new_vertices[ii].pos = vertices[ii].pos;
        new_vertices[ii].norm = vertices[ii].norm;
        new_vertices[ii].tex = vertices[ii].tex;
    }
    for(int ii=0;ii<num_indices;ii+=3) {
        uint32_t i0,i1,i2;
        if(index_size == 2) {
            i0 = ((uint16_t*)indices)[ii+0];
            i1 = ((uint16_t*)indices)[ii+1];
            i2 = ((uint16_t*)indices)[ii+2];
        } else {
            i0 = ((uint32_t*)indices)[ii+0];
            i1 = ((uint32_t*)indices)[ii+1];
            i2 = ((uint32_t*)indices)[ii+2];
        }

        VtxPosNormTanBitanTex& v0 = new_vertices[i0];
        VtxPosNormTanBitanTex& v1 = new_vertices[i1];
        VtxPosNormTanBitanTex& v2 = new_vertices[i2];

        float3 delta_pos1 = float3subtract(&v1.pos, &v0.pos);
        float3 delta_pos2 = float3subtract(&v2.pos, &v0.pos);
        float2 delta_uv1 = float2subtract(&v1.tex, &v0.tex);
        float2 delta_uv2 = float2subtract(&v2.tex, &v0.tex);

        float r = 1.0f / (delta_uv1.x * delta_uv2.y - delta_uv1.y * delta_uv2.x);
        float3 a = float3multiplyScalar(&delta_pos1, delta_uv2.y);
        float3 b = float3multiplyScalar(&delta_pos2, delta_uv1.y);
        float3 tangent = float3subtract(&a,&b);
        tangent = float3multiplyScalar(&tangent, r);

        a = float3multiplyScalar(&delta_pos2, delta_uv1.x);
        b = float3multiplyScalar(&delta_pos1, delta_uv2.x);
        float3 bitangent = float3subtract(&a,&b);
        bitangent = float3multiplyScalar(&bitangent, r);

        v0.bitan = bitangent;
        v1.bitan = bitangent;
        v2.bitan = bitangent;

        v0.tan = tangent;
        v1.tan = tangent;
        v2.tan = tangent;
    }
    return new_vertices;
}
private:
void _clear(void) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    CheckGLError();
}
void _present(void) {
#ifdef __APPLE__
    _osx_flush_buffer(_window);
#elif defined(_WIN32)
    SwapBuffers(_dc);
    CheckGLError();
#endif
}
void _load_shaders(void) {

    GLuint vs = _compile_shader(GL_VERTEX_SHADER, "assets/shaders/2D.vsh");
    GLuint fs = _compile_shader(GL_FRAGMENT_SHADER, "assets/shaders/2D.fsh");
    _2d_program = _create_program(vs, fs);
    glDeleteShader(vs);

    vs = _compile_shader(GL_VERTEX_SHADER, "assets/shaders/2D_fullscreen.vsh");
    _fullscreen_program = _create_program(vs, fs);
    glDeleteShader(fs);

    _2d_viewproj_uniform = glGetUniformLocation(_2d_program, "kViewProj");
    _2d_world_uniform = glGetUniformLocation(_2d_program, "kWorld");
}
void _create_base_meshes(void) {
    _cube_mesh = create_mesh(ARRAYSIZE(kCubeVertices), kVtxPosNormTex,
                             ARRAYSIZE(kCubeIndices), sizeof(kCubeIndices[0]),
                             kCubeVertices, kCubeIndices);
    _quad_mesh = create_mesh(ARRAYSIZE(kQuadVertices), kVtxPosTex,
                             ARRAYSIZE(kQuadIndices), sizeof(kQuadIndices[0]),
                             kQuadVertices, kQuadIndices);
    _fullscreen_quad_mesh = create_mesh(ARRAYSIZE(kFullscreenQuadVertices), kVtxPosTex,
                                        ARRAYSIZE(kQuadIndices), sizeof(kQuadIndices[0]),
                                        kFullscreenQuadVertices, kQuadIndices);
    _sphere_mesh = load_mesh("assets/sphere.mesh");
}
void _resize_framebuffer(void) {
    glBindFramebuffer(GL_FRAMEBUFFER, _frame_buffer);
    glViewport(0, 0, _width, _height);

    glBindRenderbuffer(GL_RENDERBUFFER, _color_buffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA, _width, _height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, _color_buffer);
    CheckGLError();

    glBindRenderbuffer(GL_RENDERBUFFER, _depth_buffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, _width, _height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, _depth_buffer);
    CheckGLError();

    glBindTexture(GL_TEXTURE_2D, _color_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, _width, _height, 0, GL_RGBA, GL_HALF_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _color_texture, 0);
    CheckGLError();

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if( status != GL_FRAMEBUFFER_COMPLETE) {
        debug_output("FBO initialization failed\n");
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
void _create_framebuffer(void) {

    glGenFramebuffers(1, &_frame_buffer);
    glGenRenderbuffers(1, &_color_buffer);
    glGenRenderbuffers(1, &_depth_buffer);

    glGenTextures(1, &_color_texture);
}

private:

#ifdef _WIN32
HDC     _dc;
#endif
void*   _window;

Mesh    _meshes[kMAX_MESHES];
int     _num_meshes;
MeshID  _cube_mesh;
MeshID  _quad_mesh;
MeshID  _sphere_mesh;
MeshID  _fullscreen_quad_mesh;

GLuint  _textures[kMAX_TEXTURES];
int     _num_textures;

float4x4    _perspective_projection;
float4x4    _3d_view;
float4x4    _orthographic_projection;
float4x4    _2d_view;

RenderCommand   _3d_render_commands[kMAX_RENDER_COMMANDS];
int             _num_3d_render_commands;
RenderCommand   _2d_render_commands[kMAX_RENDER_COMMANDS];
int             _num_2d_render_commands;

Renderable  _renderables[kMAX_RENDER_COMMANDS];
int         _num_renderables;

LightBuffer _light_buffer;

int _debug;
int _deferred;

GLuint  _frame_buffer;
GLuint  _color_buffer;
GLuint  _depth_buffer;

GLuint  _color_texture;

GLuint  _fullscreen_program;

GLuint  _2d_program;
GLuint  _2d_world_uniform;
GLuint  _2d_viewproj_uniform;

int     _width;
int     _height;

RendererDeferred    _deferred_renderer;
RendererForward     _forward_renderer;

};

/*
 * External
 */
Render* Render::_create_ogl(void) {
    return new RenderGL;
}
