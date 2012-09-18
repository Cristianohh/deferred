/*! @file render_gl_helper.h
 *  @brief TODO: Add the purpose of this module
 *  @author Kyle Weicht
 *  @date 9/17/12 9:03 PM
 *  @copyright Copyright (c) 2012 Kyle Weicht. All rights reserved.
 */
#ifndef __render_gl_helper_render_gl_helper__
#define __render_gl_helper_render_gl_helper__

#ifdef __cplusplus
extern "C" { // Use C linkage
#endif 

#define CheckGLError()                  \
    do {                                \
        GLenum _glError = glGetError(); \
        if(_glError != GL_NO_ERROR) {   \
            debug_output("OpenGL Error: %d\n", _glError);\
        }                               \
        assert(_glError == GL_NO_ERROR);\
    } while(__LINE__ == 0)


typedef struct {
    GLuint      vao;
    GLuint      vertex_buffer;
    GLuint      index_buffer;
    uint32_t    index_count;
    GLenum      index_format;
} Mesh;
typedef struct {
    uint32_t slot;
    int count;
} VertexDescription;

typedef struct {
    float4x4    transform;
    MeshID      mesh;
    TextureID   texture;
} RenderCommand;

static GLuint _compile_shader(GLenum shader_type, const char* filename) {
    MessageBoxResult result = kMBOK;
    GLint status = GL_TRUE;
    GLuint shader = 0;
    CheckGLError();
    do {
        result = kMBOK;
        GLchar buffer[1024*64]; // 64k should be a safe size
        FILE* file = fopen(filename, "rt");
        assert(file);
        size_t bytes_read = fread(buffer, sizeof(GLchar), sizeof(buffer), file);
        buffer[bytes_read] = '\0';
        fclose(file);

        shader = glCreateShader(shader_type);
        const GLchar* p = buffer;
        glShaderSource(shader, 1, &p, NULL);
        CheckGLError();
        glCompileShader(shader);
        CheckGLError();
        glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
        CheckGLError();
        if(status == GL_FALSE) {
            char printBuffer[1024*16];
            char statusBuffer[1024*16];
            glGetShaderInfoLog(shader, sizeof(statusBuffer), NULL, statusBuffer);
            CheckGLError();

            snprintf(printBuffer, sizeof(printBuffer), "Error compiling shader: %s\n%s", filename, statusBuffer);
            result = message_box("Shader Compile Error", printBuffer);
            glDeleteShader(shader);
            CheckGLError();
            shader = 0;
        }
    } while(result == kMBRetry);

    return shader;
}
static GLuint _create_program(GLuint vertex_shader, GLuint fragment_shader) {
    GLint status = GL_TRUE;
    GLuint program = glCreateProgram();
    CheckGLError();
    glAttachShader(program, vertex_shader);
    CheckGLError();
    glAttachShader(program, fragment_shader);
    CheckGLError();
    glLinkProgram(program);
    CheckGLError();
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    CheckGLError();
    if(status == GL_FALSE) {
        char printBuffer[1024];
        char statusBuffer[1024];
        glGetProgramInfoLog(program, sizeof(statusBuffer), NULL, statusBuffer);
        CheckGLError();

        snprintf(printBuffer, sizeof(printBuffer), "Link error: %s", statusBuffer);
        debug_output("%s\n", printBuffer);
        glDeleteProgram(program);
        CheckGLError();
        program = 0;
    }
    glUseProgram(0);
    return program;
}
static void _validate_program(GLuint program) {
    GLint status = GL_TRUE;
    glValidateProgram(program);
    CheckGLError();
    glGetProgramiv(program, GL_VALIDATE_STATUS, &status);
    CheckGLError();
    if(status == GL_FALSE) {
        char printBuffer[1024];
        char statusBuffer[1024];
        glGetProgramInfoLog(program, sizeof(statusBuffer), NULL, statusBuffer);
        CheckGLError();

        snprintf(printBuffer, sizeof(printBuffer), "Link error: %s", statusBuffer);
        debug_output("%s\n", printBuffer);
    }
}
static GLuint _create_buffer(GLenum type, size_t size, const void* data) {\
    GLuint buffer;
    glGenBuffers(1, &buffer);
    CheckGLError();
    assert(buffer);
    glBindBuffer(type, buffer);
    CheckGLError();
    glBufferData(type, (GLsizeiptr)size, data, GL_DYNAMIC_DRAW);
    CheckGLError();
    glBindBuffer(type, 0);
    CheckGLError();
    return buffer;
}
static Mesh _create_mesh(uint32_t vertex_count, size_t vertex_size,
                         uint32_t index_count, size_t index_size,
                         const void* vertices, const void* indices,
                         const VertexDescription* vertex_desc)
{
    GLuint vertex_buffer = _create_buffer(GL_ARRAY_BUFFER, vertex_count*vertex_size, vertices);
    GLuint index_buffer = _create_buffer(GL_ELEMENT_ARRAY_BUFFER, index_size*index_count, indices);
    intptr_t offset = 0;

    GLuint vao;
    glGenVertexArrays(1, &vao);
    assert(vao);
    CheckGLError();
    glBindVertexArray(vao);
    CheckGLError();
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    CheckGLError();
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);
    CheckGLError();
    while(vertex_desc && vertex_desc->count) {
        glEnableVertexAttribArray(vertex_desc->slot);
        CheckGLError();
        glVertexAttribPointer(vertex_desc->slot, vertex_desc->count, GL_FLOAT, GL_FALSE, (GLsizei)vertex_size, (void*)offset);
        CheckGLError();
        offset += sizeof(float) * (uint32_t)vertex_desc->count;
        ++vertex_desc;
    }
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    CheckGLError();

    Mesh mesh;
    mesh.index_buffer = index_buffer;
    mesh.vertex_buffer = vertex_buffer;
    mesh.index_count = index_count;
    mesh.index_format = (index_size == 2) ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT;
    mesh.vao = vao;
    return mesh;
}

#ifdef __cplusplus
}
#endif

#endif /* include guard */
