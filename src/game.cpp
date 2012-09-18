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

/*
 * Internal
 */
static float _rand_float(float min, float max) {
    float r = rand()/(float)RAND_MAX;
    float delta = max-min;
    r *= delta;
    return r+min;
}

/*
 * External
 */
Game::Game()
    : _num_objects(0)
{
    _fps.frame = 0;
    _camera = TransformZero();
    _camera.position.z = -60.0f;
    _camera.position.y = 15.0f;
    float3 axis = {1.0f, 0.0f, 0.0f};
    _camera.orientation = quaternionFromAxisAngle(&axis, 0.3f);
}
void Game::initialize(void) {
    timer_init(&_timer);
    _frame_count = 0;
    _render = Render::create();
    _render->initialize(app_get_window());

    //srand((uint32_t)_timer.start_time);

    Object o;
    // Ground
    const VtxPosNormTex ground_vertices[] =
    {
        /* Top */
        { {-0.5f,  0.5f, -0.5f}, { 0.0f,  1.0f,  0.0f}, {0.0f, 10.0f} },
        { { 0.5f,  0.5f, -0.5f}, { 0.0f,  1.0f,  0.0f}, {10.0f, 10.0f} },
        { { 0.5f,  0.5f,  0.5f}, { 0.0f,  1.0f,  0.0f}, {10.0f, 0.0f} },
        { {-0.5f,  0.5f,  0.5f}, { 0.0f,  1.0f,  0.0f}, {0.0f, 0.0f} },
    };
    const unsigned short ground_indices[] =
    {
        3,1,0,
        2,1,3,
    };
    o.transform = TransformZero();
    o.transform.scale = 100.0f;
    o.transform.position.y = -50.0f;
    o.mesh = _render->create_mesh(4, kVtxPosNormTex, 6, sizeof(ground_indices[0]), ground_vertices, ground_indices);
    o.texture = _render->load_texture("assets/grass.jpg");
    _add_object(o);

    
    TextureID textures[3] = {0};
    textures[0] = _render->load_texture("assets/metal.jpg");
    textures[1] = _render->load_texture("assets/brick.jpg");
    textures[2] = _render->load_texture("assets/wood.jpg");

    for(int ii=0; ii<32;++ii) {
        o.transform = TransformZero();
        o.transform.scale = _rand_float(0.5f, 5.0f);
        o.transform.position.x = _rand_float(-50.0f, 50.0f);
        o.transform.position.y = _rand_float(0.5f, 5.0f);
        o.transform.position.z = _rand_float(-50.0f, 50.0f);
        float3 axis = { _rand_float(-3.0f, 3.0f), _rand_float(-3.0f, 3.0f), _rand_float(-3.0f, 3.0f) };
        o.transform.orientation = quaternionFromAxisAngle(&axis, _rand_float(0.0f, kPi));
        switch(rand()%2) {
            case 0:
                o.mesh = _render->sphere_mesh();
                break;
            case 1:
                o.mesh = _render->cube_mesh();
                break;
        }
        o.texture = textures[rand()%3];
        _add_object(o);
    }

    // Add a "sun"
    for(int ii=0;ii<MAX_LIGHTS;++ii) {
        _lights[ii].x = _rand_float(-50.0f, 50.0f);
        _lights[ii].y = _rand_float(1.0f, 4.0f);
        _lights[ii].z = _rand_float(-50.0f, 50.0f);
        _lights[ii].w = 8.0f;
        _colors[ii].x = _rand_float(0.0f, 1.0f);
        _colors[ii].y = _rand_float(0.0f, 1.0f);
        _colors[ii].z = _rand_float(0.0f, 1.0f);
        _colors[ii].w = 1.0f;
    }
}
void Game::shutdown(void) {
    _render->shutdown();
    Render::destroy(_render);
}
int Game::on_frame(void) {
    // Handle OS events
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
        default:
            break;
        }
        event = app_pop_event();
    }

    // Beginning of frame stuff
    _delta_time = (float)timer_delta_time(&_timer);
    update_fps(&_fps, _delta_time);


    // Frame
    _control_camera();
    float4x4 view = TransformGetMatrix(&_camera);
    _render->set_3d_view_matrix(float4x4inverse(&view));

    for(int ii=0;ii<_num_objects;++ii) {
        const Object& o = _objects[ii];
        _render->draw_3d(o.mesh, o.texture, TransformGetMatrix(&o.transform));
    }
    for(int ii=0;ii<MAX_LIGHTS;++ii) {
        _render->draw_light(_lights[ii], _colors[ii]);
    }
    _render->render();

    // End of frame stuff
    if(++_frame_count % 16 == 0)
        debug_output("%.1f FPS\n", get_fps(&_fps));
    return 0;
}
void Game::_control_camera()
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
}
void Game::_add_object(const Object& o) {
    _objects[_num_objects++] = o;
}
