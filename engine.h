#ifndef ENGINE_H
#define ENGINE_H

#include "actor.h"
#include "audio.h"
#include "camera2d.h"
#include "directional_light3d.h"
#include "LuaBridge/LuaBridge.h"
#include "box2d/box2d.h"
#include "input.h"
#include "renderer2d.h"
#include "renderer3d.h"
#include "ui_system.h"
#include <filesystem>
#include <string>
#include <unordered_map>
#include <iostream>
#include <cstdlib>
#include "scene_DB.h"
#include "actor_template_DB.h"

struct SDL_Window;

enum class RenderMode {
    Mode2D,
    Mode3D
};

class engine{
public:
    // Engine
    inline static engine* current_engine = nullptr;
    bool game_running = 1;
    std::string game_title = "";
    lua_State* lua_state = nullptr;
    float pixels_per_unit = 100.0f;
    float camera_w = 640.0f;
    float camera_h = 360.0f;
    float delta_time = 1.0f / 60.0f;
    Uint32 previous_frame_timestamp = 0;
    camera2d my_camera2d;
    renderer2d my_renderer2d;
    renderer3d my_renderer3d;
    ui_system my_ui_system;
    RenderMode render_mode = RenderMode::Mode2D;
    void initialize();
    void game_loop();
    void input();
    void update();
    void render();
    void end();
    static float GetDeltaTime(); //api
    static float GetScreenWidth(); //api
    static float GetScreenHeight(); //api

    // Scene
    std::string initial_scene;
    scene my_scene;
    actor_template my_template;
};

#endif // !ENGINE_H
