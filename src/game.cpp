/*! @file game.cpp
 *  @author kcweicht
 *  @date 9/14/2012 2:16:40 PM
 *  @copyright Copyright (c) 2012 kcweicht. All rights reserved.
 */
#include "game.h"

#include <stdio.h>
#include <stdlib.h>
#include "render.h"
#include "assert.h"
#include "application.h"
#include "unit_test.h"
#include "marching_cubes.h"
#include "perlin_noise.h"

/*
 * Internal
 */
namespace {

float _rand_float(float min, float max) {
    float r = rand()/(float)RAND_MAX;
    float delta = max-min;
    r *= delta;
    return r+min;
}


float terrain_func(float3 v) {
    float density = -v.y;
    density += (float)noise(2, v.x*0.125f, v.y*0.125f, v.z*0.125f) * 8.0f;
    density += (float)noise(32, v.x, v.y, v.z);
    density += (float)noise(54, v.x*2, v.y*2, v.z*2) * 0.5f;
    density += (float)noise(78, v.x*4, v.y*4, v.z*4) * 0.25f;
    return density;
}

struct RenderData {
    Resource    mesh;
    Material    material;

    static Render*  _render;
};
Render* RenderData::_render = NULL;

typedef SimpleComponent<RenderData, kRenderComponent>   RenderComponent;
typedef SimpleSystem<RenderData>                        RenderSystem;

struct LightData {
    LightData() { }
    LightData(Light _light) : light(_light) { }
    Light   light;

    static Render*  _render;
};

Render* LightData::_render = NULL;

typedef SimpleComponent<LightData, kLightComponent>   LightComponent;
typedef SimpleSystem<LightData>                        LightSystem;

void smooth_terrain(std::vector<float3>& positions, std::vector<VtxPosNormTex>& vertices, std::vector<uint32_t>& indices) {
    std::map<std::pair<float, std::pair<float,float> >, uint32_t> index_map;
    for(uint32_t ii=0; ii<positions.size(); ii += 3) {
        triangle_t tri = {
            positions[ii+0],
            positions[ii+1],
            positions[ii+2],
        };
        float3 norm;
        float3 edge0 = float3subtract(&tri.p[1], &tri.p[0]);
        float3 edge1 = float3subtract(&tri.p[2], &tri.p[0]);
        norm = float3cross(&edge1, &edge0);
        norm = float3cross(&edge0, &edge1);
        norm = float3normalize(&norm);
        for(int jj=0;jj<3;++jj) {
            std::pair<float, std::pair<float,float> > check = std::make_pair(tri.p[jj].x, std::make_pair(tri.p[jj].y, tri.p[jj].z));
            std::map<std::pair<float, std::pair<float,float> >, uint32_t>::iterator iter = index_map.find(check);
            if(iter != index_map.end()) { // If we've already added this vertex, add the normal and add to the index list
                vertices[iter->second].norm = float3add(&norm, &vertices[iter->second].norm);
                indices.push_back(iter->second);
            } else {
                VtxPosNormTex vtx = {
                    tri.p[jj],
                    norm,
                    {0.0f, 0.0f},
                };
                vertices.push_back(vtx);
                uint32_t index = (uint32_t)vertices.size()-1;
                indices.push_back(index);
                index_map[check] = index;
            }
        }
    }

    for(uint32_t ii=0;ii<vertices.size();++ii) {
        vertices[ii].norm = float3normalize(&vertices[ii].norm);
        vertices[ii].tex.x = vertices[ii].pos.x;
        vertices[ii].tex.y = vertices[ii].pos.z;
    }
}

}

template<> void SimpleSystem<RenderData>::_update(Entity* entity, RenderData* data, float) {
    Transform transform = entity->transform();
    data->_render->draw_3d(data->mesh, &data->material, TransformGetMatrix(&transform));
}

template<> void SimpleSystem<LightData>::_update(Entity* entity, LightData* data, float) {
    Transform transform = entity->transform();
    data->light.pos = transform.position;
    data->light.dir = quaternionGetZAxis(&transform.orientation);
    data->_render->draw_light(data->light);
}


/*
 * External
 */
Game::Game()
{
    _fps.frame = 0;
    _camera = TransformZero();
    //_camera.position.z = -60.0f;
    _camera.position.y = 5.0f;
    float3 axis = {1.0f, 0.0f, 0.0f};
    _camera.orientation = quaternionFromAxisAngle(&axis, 0.3f);
}
void Game::initialize(void) {

    timer_init(&_timer);
    _frame_count = 0;
    _render = Render::create();
    _render->initialize(app_get_window());

    _world.add_system(new RenderSystem, kRenderComponent);
    _world.add_system(new LightSystem, kLightComponent);
    RenderData::_render = _render;
    LightData::_render = _render;


    //srand((uint32_t)_timer.start_time);
    srand(42);

    // Texture loaders
    _resource_manager.add_handlers("jpg", Render::load_texture, Render::unload_texture, _render);
    _resource_manager.add_handlers("dds", Render::load_texture, Render::unload_texture, _render);
    _resource_manager.add_handlers("png", Render::load_texture, Render::unload_texture, _render);
    _resource_manager.add_handlers("tga", Render::load_texture, Render::unload_texture, _render);

    // File loaders
    _resource_manager.add_handlers("mesh", Render::load_mesh, Render::unload_mesh, _render);
    _resource_manager.add_handlers("obj", Render::load_mesh, Render::unload_mesh, _render);

    // Create some materials
    Material grass_material =
    {
        _resource_manager.get_resource("assets/grass.dds"),
        _resource_manager.get_resource("assets/grass_nrm.png"),
        {0},
        {0.0f, 0.0f, 0.0f},
        0.0f,
        0.0f
    };

    Material materials[3] =
    {
        {
            _resource_manager.get_resource("assets/metal.dds"),
            _resource_manager.get_resource("assets/metal_nrm.png"),
            _resource_manager.get_resource("assets/metal.dds"),
            {0.0f, 0.0f, 0.0f},
            200.0f,
            0.8f
        },
        {
            _resource_manager.get_resource("assets/brick.dds"),
            _resource_manager.get_resource("assets/brick_nrm.png"),
            0,
            {1.0f, 1.0f, 1.0f},
            4.0f,
            0.1f
        },
        {
            _resource_manager.get_resource("assets/wood.dds"),
            _resource_manager.get_resource("assets/wood_nrm.png"),
            0,
            {1.0f, 1.0f, 1.0f},
            8.0f,
            0.3f
        }
    };

    // Ground
    const VtxPosNormTex ground_vertices[] =
    {
        /* Top */
        { {-0.5f,  0.0f, -0.5f}, { 0.0f,  1.0f,  0.0f}, {0.0f, 10.0f} },
        { { 0.5f,  0.0f, -0.5f}, { 0.0f,  1.0f,  0.0f}, {10.0f, 10.0f} },
        { { 0.5f,  0.0f,  0.5f}, { 0.0f,  1.0f,  0.0f}, {10.0f, 0.0f} },
        { {-0.5f,  0.0f,  0.5f}, { 0.0f,  1.0f,  0.0f}, {0.0f, 0.0f} },
        /* Bottom */
        { {-0.5f,  0.0f, -0.5f}, { 0.0f,  -1.0f,  0.0f}, {0.0f, 10.0f} },
        { { 0.5f,  0.0f, -0.5f}, { 0.0f,  -1.0f,  0.0f}, {10.0f, 10.0f} },
        { { 0.5f,  0.0f,  0.5f}, { 0.0f,  -1.0f,  0.0f}, {10.0f, 0.0f} },
        { {-0.5f,  0.0f,  0.5f}, { 0.0f,  -1.0f,  0.0f}, {0.0f, 0.0f} },
    };
    const unsigned short ground_indices[] =
    {
        3,1,0,
        2,1,3,
        3,0,1,
        2,3,1,
    };
    assert(ground_indices && ground_vertices);

    Transform transform = TransformZero();
    transform.scale = 1.0f;
    transform.position.y = -100000.0f;
    transform.position.y = 0.0f;
    RenderData render_data = {0};
    //render_data.mesh = _render->create_mesh(ARRAYSIZE(ground_vertices), kVtxPosNormTex, ARRAYSIZE(ground_indices), sizeof(ground_indices[0]), ground_vertices, ground_indices);
    //render_data.mesh = _render->sphere_mesh();
    std::map<float3, VtxPosNormTex> m;
    std::vector<float3> verts;
    std::vector<VtxPosNormTex>  terrain_verts;
    std::vector<uint32_t>       terrain_indices;
    verts.reserve(1000000);
    terrain_verts.reserve(1000000);
    terrain_indices.reserve(1000000);
    generate_terrain_points(terrain_func, verts);  
    debug_output("Num raw Vertices: %d\n", verts.size());
    smooth_terrain(verts, terrain_verts, terrain_indices);
    //generate_terrain(terrain_func, terrain_verts, terrain_indices);
    debug_output("Num Terrain Vertices: %d\n", terrain_verts.size());
    debug_output("Num Terrain indices: %d\n", terrain_indices.size());
    render_data.mesh = _render->create_mesh((uint32_t)terrain_verts.size(), kVtxPosNormTex, (uint32_t)terrain_indices.size(), sizeof(uint32_t), terrain_verts.data(), terrain_indices.data());
    render_data.material = grass_material;

    EntityID id = _world.create_entity();
    _world.entity(id)->set_transform(transform)
                     ->add_component(RenderComponent(render_data));


    for(int ii=0; ii<32;++ii) {
        transform = TransformZero();
        transform.scale = _rand_float(0.5f, 5.0f);
        transform.position.x = _rand_float(-50.0f, 50.0f);
        transform.position.y = _rand_float(3.0f, 7.5f);
        transform.position.z = _rand_float(-50.0f, 50.0f);
        float3 axis = { _rand_float(-3.0f, 3.0f), _rand_float(-3.0f, 3.0f), _rand_float(-3.0f, 3.0f) };
        transform.orientation = quaternionFromAxisAngle(&axis, _rand_float(0.0f, kPi));
        switch(rand()%2) {
            case 0:
                render_data.mesh = _render->sphere_mesh();
                break;
            case 1:
                render_data.mesh = _render->cube_mesh();
                break;
        }
        int material = rand()%3;
        render_data.material = materials[material];
        id = _world.create_entity();
        _world.entity(id)->set_transform(transform);
                         //->add_component(RenderComponent(render_data));
    }

    Material house_material =
    {
        _resource_manager.get_resource("assets/house_diffuse.tga"),
        _resource_manager.get_resource("assets/house_normal.tga"),
        _resource_manager.get_resource("assets/house_spec.tga"),
        {0.0f, 0.0f, 0.0f},
        1.5f,
        0.06f
    };
    transform = TransformZero();
    transform.scale = 0.015f;
    transform.orientation = quaternionFromEuler(0.0f, DegToRad(-90.0f), 0.0f);

    render_data.mesh = _resource_manager.get_resource("assets/house_obj.obj");
    render_data.material = house_material;
    id = _world.create_entity();
    _world.entity(id)->set_transform(transform);
                     //->add_component(RenderComponent(render_data));

    // Add a "sun"
    Light light;
    light.pos.x = 0.0f;
    light.pos.y = 10.0f;
    light.pos.z = 0.0f;
    light.size = 250.0f;
    light.dir.x = 0.0f;
    light.dir.y = -1.0f;
    light.dir.z = 0.0f;
    light.color.x = 1.0f;
    light.color.y = 1.0f;
    light.color.z = 1.0f;
    light.inner_cos = 0.1f;
    light.type = kDirectionalLight;

    transform = TransformZero();
    transform.orientation = quaternionFromEuler(DegToRad(90.0f), DegToRad(60.0f), 0.0f);
    transform.position = light.pos;

    _sun_id = _world.create_entity();
    _world.entity(_sun_id)->set_transform(transform)
                          ->add_component(LightComponent(light));

    transform = TransformZero();
    for(int ii=1;ii<MAX_LIGHTS;++ii) {
        light.pos.x = _rand_float(-50.0f, 50.0f);
        light.pos.y = _rand_float(1.0f, 4.0f);
        light.pos.z = _rand_float(-50.0f, 50.0f);
        light.size = 3.0f;
        light.color.x = _rand_float(0.0f, 1.0f);
        light.color.y = _rand_float(0.0f, 1.0f);
        light.color.z = _rand_float(0.0f, 1.0f);
        light.type = kPointLight;

        transform.position = light.pos;
        id = _world.create_entity();
        _world.entity(id)->set_transform(transform);
                         //->add_component(LightComponent(light));
    }
}
void Game::shutdown(void) {
    app_unlock_and_show_cursor();
    _render->shutdown();
    Render::destroy(_render);
}
int Game::on_frame(void) {
    // Handle OS events
    float delta_x = 0.0f;
    float delta_y = 0.0f;
    const SystemEvent* event = app_pop_event();
    while (event) {
        switch(event->type) {
            case kEventResize:
                _render->resize(event->data.resize.width, event->data.resize.height);
                printf("W: %d  H: %d\n", event->data.resize.width, event->data.resize.height);
                break;
            case kEventKeyDown:
                if(event->data.key == KEY_ESCAPE)
                    return 1;
                if(event->data.key == KEY_F1)
                    _render->toggle_debug_graphics();
                if(event->data.key == KEY_F2)
                    _render->toggle_deferred();
                break;
            case kEventMouseDown:
                if(event->data.mouse.button == MOUSE_LEFT)
                    app_lock_and_hide_cursor();
                debug_output("Mouse: %.0f %.0f\n", event->data.mouse.x, event->data.mouse.y);
                break;
            case kEventMouseUp:
                if(event->data.mouse.button == MOUSE_LEFT)
                    app_unlock_and_show_cursor();
                break;
            case kEventMouseMove:
                delta_x += event->data.mouse.x;
                delta_y += event->data.mouse.y;
                break;
            default:
                break;
        }
        event = app_pop_event();
    }

    // Beginning of frame stuff
    _delta_time = (float)timer_delta_time(&_timer);
    update_fps(&_fps, _delta_time);

    float3 xaxis = {1.0f, 0.0f, 0.0f};
    quaternion q = quaternionFromAxisAngle(&xaxis, _delta_time*0.15f);
    Transform t = _world.entity(_sun_id)->transform();
    t.orientation = quaternionMultiply(&q, &t.orientation);
    //_world.entity(_sun_id)->set_transform(t);
    if(quaternionGetZAxis(&t.orientation).y > 0.0f) {
        //_world.entity(_sun_id)->deactivate_component(kLightComponent);
    } else {
        //_world.entity(_sun_id)->activate_component(kLightComponent);
    }

    // Frame
    _control_camera(delta_x, delta_y);
    float4x4 view = TransformGetMatrix(&_camera);
    _render->set_3d_view_matrix(view);

    _world.update(_delta_time);

    _render->render();

    // End of frame stuff
    if(++_frame_count % 16 == 0) {
        debug_output("%.2fms (%.0f FPS)\n", get_frametime(&_fps), get_fps(&_fps));
    }
    return 0;
}
void Game::_control_camera(float mouse_x, float mouse_y)
{
    float camera_speed = 5.0f * _delta_time;
    if(app_is_key_down(KEY_SHIFT))
        camera_speed *= 3.0f;

    float3 look = quaternionGetZAxis(&_camera.orientation);
    float3 right = quaternionGetXAxis(&_camera.orientation);
    float3 up = quaternionGetYAxis(&_camera.orientation);
    look = float3multiplyScalar(&look, camera_speed);
    up = float3multiplyScalar(&up, camera_speed);
    right = float3multiplyScalar(&right, camera_speed);

    if(app_is_key_down(KEY_W))
        _camera.position = float3add(&_camera.position, &look);
    if(app_is_key_down(KEY_S))
        _camera.position = float3subtract(&_camera.position, &look);

    if(app_is_key_down(KEY_D))
        _camera.position = float3add(&_camera.position, &right);
    if(app_is_key_down(KEY_A))
        _camera.position = float3subtract(&_camera.position, &right);

    if(app_is_key_down(KEY_E))
        _camera.position = float3add(&_camera.position, &up);
    if(app_is_key_down(KEY_Q))
        _camera.position = float3subtract(&_camera.position, &up);

    if(app_is_key_down(KEY_UP))
        mouse_y = -1.0f;
    else if(app_is_key_down(KEY_DOWN))
        mouse_y = 1.0f;

    if(app_is_key_down(KEY_RIGHT))
        mouse_x = 1.0f;
    else if(app_is_key_down(KEY_LEFT))
        mouse_x = -1.0f;

    if(app_is_mouse_button_down(MOUSE_LEFT) == 0)
        mouse_x = mouse_y = 0.0f;

    // L/R Rotation
    float3 yaxis = {0.0f, 1.0f, 0.0f};
    quaternion q = quaternionFromAxisAngle(&yaxis, _delta_time*mouse_x);
    _camera.orientation = quaternionMultiply(&_camera.orientation, &q);

    // U/D rotation
    float3 xaxis = {1.0f, 0.0f, 0.0f};
    q = quaternionFromAxisAngle(&xaxis, _delta_time*mouse_y);
    _camera.orientation = quaternionMultiply(&q, &_camera.orientation);
}
