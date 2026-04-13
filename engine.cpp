#include "engine.h"
#include "math_utils.h"
#include "particle.h"
#include <algorithm>

void engine::game_loop(){
    initialize();
    previous_frame_timestamp = SDL_GetTicks();
    while(game_running){
        const Uint32 now = SDL_GetTicks();
        delta_time = static_cast<float>(now - previous_frame_timestamp) / 1000.0f;
        previous_frame_timestamp = now;
        input();
        update();
        render();
    }
    end();
}

void engine::input(){
    SDL_Event event;
    while (Helper::SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT)
            game_running = false;
        Input::ProcessEvent(event);
    }
}

void engine::update(){
    if (my_scene.has_new_scene) {
        my_scene.has_new_scene = false;
        my_scene.load_scene(my_scene.new_scene_name, *my_scene.templates);
        my_scene.new_scene_name.clear();
    }
    physics2d::EnsureWorld(my_scene);
    physics3d::EnsureWorld(my_scene);
    my_scene.process_onstart();
    my_scene.process_onupdate();
    my_scene.process_onlateupdate();
    my_scene.process_ondestroy();
    my_scene.process_event_subscriptions_changes();
    physics2d::StepWorld(my_scene);
    physics3d::StepWorld(my_scene);
    my_scene.cleanup_end_of_frame();
    Input::LateUpdate();
}

void engine::render(){
    if (render_mode == RenderMode::Mode3D)
        my_renderer3d.render();
    else
        my_renderer2d.render();
}

float engine::GetDeltaTime() {
    if (current_engine == nullptr)
        return 0.0f;
    return current_engine->delta_time;
}

float engine::GetScreenWidth() {
    if (current_engine == nullptr)
        return 0.0f;
    return current_engine->camera_w;
}

float engine::GetScreenHeight() {
    if (current_engine == nullptr)
        return 0.0f;
    return current_engine->camera_h;
}

void engine::initialize(){
    current_engine = this;
    my_ui_system.reserve();
    camera2d::current_camera2d = &my_camera2d;
    if (!std::filesystem::exists("resources/")){
        std::cout << "error: resources/ missing";
        exit(0);
    }
    const std::string config_path_game = "resources/game.config";
    if (!std::filesystem::exists(config_path_game)){
        std::cout << "error: " << config_path_game << " missing";
        exit(0);
    }
    
    rapidjson::Document json_game;
    Helper::ReadJsonFile(config_path_game, json_game);
    if (json_game.HasMember("game_title"))
        game_title = json_game["game_title"].GetString();
    if (!json_game.HasMember("initial_scene")) {
        std::cout << "error: initial_scene unspecified";
        exit(0);
    }
    initial_scene = json_game["initial_scene"].GetString();
    if (json_game.HasMember("render_mode") && json_game["render_mode"].IsString()) {
        const std::string configured_render_mode = json_game["render_mode"].GetString();
        render_mode = (configured_render_mode == "3d") ? RenderMode::Mode3D : RenderMode::Mode2D;
    } else {
        render_mode = RenderMode::Mode2D;
    }
    
    std::string config_path_rendering = "resources/rendering.config";
    rapidjson::Document json_rendering;
    if (std::filesystem::exists(config_path_rendering)){
        Helper::ReadJsonFile(config_path_rendering, json_rendering);
        if (json_rendering.HasMember("x_resolution"))
            camera_w = json_rendering["x_resolution"].GetFloat();
        if (json_rendering.HasMember("y_resolution"))
            camera_h = json_rendering["y_resolution"].GetFloat();
        if (json_rendering.HasMember("clear_color_r")) {
            const std::uint8_t r = static_cast<std::uint8_t>(json_rendering["clear_color_r"].GetInt());
            my_renderer2d.clear_color_r = r;
            my_renderer3d.clear_color.r = static_cast<float>(r) / 255.0f;
        }
        if (json_rendering.HasMember("clear_color_g")) {
            const std::uint8_t g = static_cast<std::uint8_t>(json_rendering["clear_color_g"].GetInt());
            my_renderer2d.clear_color_g = g;
            my_renderer3d.clear_color.g = static_cast<float>(g) / 255.0f;
        }
        if (json_rendering.HasMember("clear_color_b")) {
            const std::uint8_t b = static_cast<std::uint8_t>(json_rendering["clear_color_b"].GetInt());
            my_renderer2d.clear_color_b = b;
            my_renderer3d.clear_color.b = static_cast<float>(b) / 255.0f;
        }
        if (json_rendering.HasMember("zoom_factor"))
            my_camera2d.SetZoom(json_rendering["zoom_factor"].GetFloat());
    }

    lua_state = luaL_newstate();
    if (lua_state == nullptr) {
        std::cout << "error: failed to initialize lua";
        exit(0);
    }
    luaL_openlibs(lua_state);
    my_scene.lua_state = lua_state;
    luabridge::getGlobalNamespace(lua_state)
        .beginClass<glm::vec2>("vec2")
            .addConstructor<void(*)(float, float)>()
            .addProperty("x", &glm::vec2::x)
            .addProperty("y", &glm::vec2::y)
            .addFunction("__add", &Math::Vec2OperatorAdd)
            .addFunction("__sub", &Math::Vec2OperatorSubtract)
            .addFunction("__mul", &Math::Vec2OperatorMultiply)
            .addStaticFunction("Add", &Math::Vec2Add)
            .addStaticFunction("Subtract", &Math::Vec2Subtract)
            .addStaticFunction("Multiply", &Math::Vec2Multiply)
            .addStaticFunction("Length", &Math::Vec2Length)
            .addStaticFunction("Normalize", &Math::Vec2Normalize)
            .addStaticFunction("Distance", &Math::Vec2Distance)
            .addStaticFunction("Dot", &Math::Vec2Dot)
        .endClass()
        .beginClass<b2Vec2>("Vector2")
            .addConstructor<void(*)(float, float)>()
            .addProperty("x", &b2Vec2::x)
            .addProperty("y", &b2Vec2::y)
            .addFunction("Normalize", &b2Vec2::Normalize)
            .addFunction("Length", &b2Vec2::Length)
            .addFunction("__add", &b2Vec2::operator_add)
            .addFunction("__sub", &b2Vec2::operator_sub)
            .addFunction("__mul", &b2Vec2::operator_mul)
            .addStaticFunction("Distance", &Math::Vector2Distance)
            .addStaticFunction("Dot", &Math::Vector2Dot)
        .endClass()
        .beginClass<collision>("Collision")
            .addProperty("other", &collision::other)
            .addProperty("point", &collision::point)
            .addProperty("relative_velocity", &collision::relative_velocity)
            .addProperty("normal", &collision::normal)
        .endClass()
        .beginClass<collision3d>("Collision3D")
            .addProperty("other", &collision3d::other)
            .addProperty("point", &collision3d::point)
            .addProperty("relative_velocity", &collision3d::relative_velocity)
            .addProperty("normal", &collision3d::normal)
        .endClass()
        .beginClass<Rigidbody>("Rigidbody")
            .addProperty("actor", &Rigidbody::owner)
            .addProperty("key", &Rigidbody::key)
            .addProperty("enabled", &Rigidbody::enabled)
            .addProperty("x", &Rigidbody::x)
            .addProperty("y", &Rigidbody::y)
            .addProperty("body_type", &Rigidbody::body_type)
            .addProperty("precise", &Rigidbody::precise)
            .addProperty("gravity_scale", &Rigidbody::gravity_scale)
            .addProperty("density", &Rigidbody::density)
            .addProperty("angular_friction", &Rigidbody::angular_friction)
            .addProperty("rotation", &Rigidbody::rotation)
            .addProperty("has_collider", &Rigidbody::has_collider)
            .addProperty("has_trigger", &Rigidbody::has_trigger)
            .addProperty("collider_type", &Rigidbody::collider_type)
            .addProperty("width", &Rigidbody::width)
            .addProperty("height", &Rigidbody::height)
            .addProperty("radius", &Rigidbody::radius)
            .addProperty("friction", &Rigidbody::friction)
            .addProperty("bounciness", &Rigidbody::bounciness)
            .addProperty("trigger_type", &Rigidbody::trigger_type)
            .addProperty("trigger_width", &Rigidbody::trigger_width)
            .addProperty("trigger_height", &Rigidbody::trigger_height)
            .addProperty("trigger_radius", &Rigidbody::trigger_radius)
            .addFunction("GetPosition", &Rigidbody::GetPosition)
            .addFunction("GetRotation", &Rigidbody::GetRotation)
            .addFunction("SetPosition", &Rigidbody::SetPosition)
            .addFunction("SetRotation", &Rigidbody::SetRotation)
            .addFunction("GetVelocity", &Rigidbody::GetVelocity)
            .addFunction("SetVelocity", &Rigidbody::SetVelocity)
            .addFunction("GetAngularVelocity", &Rigidbody::GetAngularVelocity)
            .addFunction("SetAngularVelocity", &Rigidbody::SetAngularVelocity)
            .addFunction("GetGravityScale", &Rigidbody::GetGravityScale)
            .addFunction("SetGravityScale", &Rigidbody::SetGravityScale)
            .addFunction("GetRightDirection", &Rigidbody::GetRightDirection)
            .addFunction("SetRightDirection", &Rigidbody::SetRightDirection)
            .addFunction("GetUpDirection", &Rigidbody::GetUpDirection)
            .addFunction("SetUpDirection", &Rigidbody::SetUpDirection)
            .addFunction("AddForce", &Rigidbody::AddForce)
        .endClass()
        .beginClass<Rigidbody3D>("Rigidbody3D")
            .addProperty("actor", &Rigidbody3D::owner)
            .addProperty("key", &Rigidbody3D::key)
            .addProperty("enabled", &Rigidbody3D::enabled)
            .addProperty("position", &Rigidbody3D::position)
            .addProperty("rotation_degrees", &Rigidbody3D::rotation_degrees)
            .addProperty("body_type", &Rigidbody3D::body_type)
            .addProperty("collider_type", &Rigidbody3D::collider_type)
            .addProperty("precise", &Rigidbody3D::precise)
            .addProperty("mass", &Rigidbody3D::mass)
            .addProperty("gravity_scale", &Rigidbody3D::gravity_scale)
            .addProperty("linear_damping", &Rigidbody3D::linear_damping)
            .addProperty("angular_damping", &Rigidbody3D::angular_damping)
            .addProperty("width", &Rigidbody3D::width)
            .addProperty("height", &Rigidbody3D::height)
            .addProperty("depth", &Rigidbody3D::depth)
            .addProperty("radius", &Rigidbody3D::radius)
            .addProperty("friction", &Rigidbody3D::friction)
            .addProperty("bounciness", &Rigidbody3D::bounciness)
            .addFunction("GetPosition", &Rigidbody3D::GetPosition)
            .addFunction("GetRotation", &Rigidbody3D::GetRotation)
            .addFunction("SetPosition", &Rigidbody3D::SetPosition)
            .addFunction("SetRotation", &Rigidbody3D::SetRotation)
            .addFunction("GetVelocity", &Rigidbody3D::GetVelocity)
            .addFunction("SetVelocity", &Rigidbody3D::SetVelocity)
            .addFunction("GetAngularVelocity", &Rigidbody3D::GetAngularVelocity)
            .addFunction("SetAngularVelocity", &Rigidbody3D::SetAngularVelocity)
            .addFunction("GetGravityScale", &Rigidbody3D::GetGravityScale)
            .addFunction("SetGravityScale", &Rigidbody3D::SetGravityScale)
            .addFunction("AddForce", &Rigidbody3D::AddForce)
        .endClass()
        .beginClass<ParticleSystem>("ParticleSystem")
            .addProperty("actor", &ParticleSystem::owner)
            .addProperty("key", &ParticleSystem::key)
            .addProperty("enabled", &ParticleSystem::enabled)
            .addProperty("x", &ParticleSystem::x)
            .addProperty("y", &ParticleSystem::y)
            .addProperty("emit_angle_min", &ParticleSystem::emit_angle_min)
            .addProperty("emit_angle_max", &ParticleSystem::emit_angle_max)
            .addProperty("emit_radius_min", &ParticleSystem::emit_radius_min)
            .addProperty("emit_radius_max", &ParticleSystem::emit_radius_max)
            .addProperty("frames_between_bursts", &ParticleSystem::frames_between_bursts)
            .addProperty("burst_quantity", &ParticleSystem::burst_quantity)
            .addProperty("duration_frames", &ParticleSystem::duration_frames)
            .addProperty("start_scale_min", &ParticleSystem::start_scale_min)
            .addProperty("start_scale_max", &ParticleSystem::start_scale_max)
            .addProperty("start_speed_min", &ParticleSystem::start_speed_min)
            .addProperty("start_speed_max", &ParticleSystem::start_speed_max)
            .addProperty("rotation_min", &ParticleSystem::rotation_min)
            .addProperty("rotation_max", &ParticleSystem::rotation_max)
            .addProperty("rotation_speed_min", &ParticleSystem::rotation_speed_min)
            .addProperty("rotation_speed_max", &ParticleSystem::rotation_speed_max)
            .addProperty("gravity_scale_x", &ParticleSystem::gravity_scale_x)
            .addProperty("gravity_scale_y", &ParticleSystem::gravity_scale_y)
            .addProperty("drag_factor", &ParticleSystem::drag_factor)
            .addProperty("angular_drag_factor", &ParticleSystem::angular_drag_factor)
            .addProperty("end_scale", &ParticleSystem::end_scale)
            .addProperty("start_color_r", &ParticleSystem::start_color_r)
            .addProperty("start_color_g", &ParticleSystem::start_color_g)
            .addProperty("start_color_b", &ParticleSystem::start_color_b)
            .addProperty("start_color_a", &ParticleSystem::start_color_a)
            .addProperty("end_color_r", &ParticleSystem::end_color_r)
            .addProperty("end_color_g", &ParticleSystem::end_color_g)
            .addProperty("end_color_b", &ParticleSystem::end_color_b)
            .addProperty("end_color_a", &ParticleSystem::end_color_a)
            .addProperty("image", &ParticleSystem::image)
            .addProperty("sorting_order", &ParticleSystem::sorting_order)
            .addFunction("Stop", &ParticleSystem::Stop)
            .addFunction("Play", &ParticleSystem::Play)
            .addFunction("Burst", &ParticleSystem::Burst)
        .endClass()
        .beginClass<glm::vec3>("Vector3")
            .addConstructor<void(*)(float, float, float)>()
            .addProperty("x", &glm::vec3::x)
            .addProperty("y", &glm::vec3::y)
            .addProperty("z", &glm::vec3::z)
            .addFunction("__add", &Math::Vector3OperatorAdd)
            .addFunction("__sub", &Math::Vector3OperatorSubtract)
            .addFunction("__mul", &Math::Vector3OperatorMultiply)
            .addStaticFunction("Add", &Math::Vector3Add)
            .addStaticFunction("Subtract", &Math::Vector3Subtract)
            .addStaticFunction("Multiply", &Math::Vector3Multiply)
            .addStaticFunction("MultiplyScalarLeft", &Math::Vector3MultiplyScalarLeft)
            .addStaticFunction("Length", &Math::Vector3Length)
            .addStaticFunction("Normalize", &Math::Vector3Normalize)
            .addStaticFunction("Dot", &Math::Vector3Dot)
            .addStaticFunction("Cross", &Math::Vector3Cross)
            .addStaticFunction("Distance", &Math::Vector3Distance)
        .endClass()
        .beginClass<Transform3D>("Transform3D")
            .addConstructor<void(*)()>()
            .addProperty("position", &Transform3D::position)
            .addProperty("rotation_degrees", &Transform3D::rotation_degrees)
            .addProperty("scale", &Transform3D::scale)
            .addFunction("GetRightVector", &Transform3D::GetRightVector)
            .addFunction("GetUpVector", &Transform3D::GetUpVector)
            .addFunction("GetForwardVector", &Transform3D::GetForwardVector)
        .endClass()
        .beginClass<Camera3D>("Camera3D")
            .addConstructor<void(*)()>()
            .addProperty("transform", &Camera3D::transform)
            .addProperty("field_of_view_degrees", &Camera3D::field_of_view_degrees)
            .addProperty("aspect_ratio", &Camera3D::aspect_ratio)
            .addProperty("near_clip", &Camera3D::near_clip)
            .addProperty("far_clip", &Camera3D::far_clip)
            .addProperty("is_orthographic", &Camera3D::is_orthographic)
            .addProperty("orthographic_height", &Camera3D::orthographic_height)
        .endClass()
        .beginClass<DirectionalLight3D>("DirectionalLight3D")
            .addConstructor<void(*)()>()
            .addProperty("transform", &DirectionalLight3D::transform)
            .addProperty("color", &DirectionalLight3D::color)
            .addProperty("intensity", &DirectionalLight3D::intensity)
            .addFunction("GetDirection", &DirectionalLight3D::GetDirection)
            .addFunction("GetRadiance", &DirectionalLight3D::GetRadiance)
            .addFunction("SetDirection", &DirectionalLight3D::SetDirection)
        .endClass()
        .beginClass<Mesh3D>("Mesh3D")
            .addConstructor<void(*)()>()
            .addProperty("name", &Mesh3D::name)
            .addFunction("Clear", &Mesh3D::Clear)
            .addFunction("LoadObj", static_cast<bool (Mesh3D::*)(const std::string&)>(&Mesh3D::LoadObj))
            .addFunction("HasVertices", &Mesh3D::HasVertices)
            .addFunction("HasIndices", &Mesh3D::HasIndices)
            .addFunction("GetTriangleCount", &Mesh3D::GetTriangleCount)
        .endClass()
        .beginClass<Material3D>("Material3D")
            .addConstructor<void(*)()>()
            .addProperty("image_name", &Material3D::image_name)
            .addProperty("tint", &Material3D::tint)
            .addProperty("alpha", &Material3D::alpha)
            .addProperty("unlit", &Material3D::unlit)
        .endClass()
        .beginNamespace("Renderer3D")
            .addFunction("SubmitMesh", +[](const Mesh3D& mesh,
                                           const Transform3D& transform,
                                           const Camera3D& camera,
                                           const DirectionalLight3D& light,
                                           const Material3D& material) {
                if (engine::current_engine == nullptr || engine::current_engine->render_mode != RenderMode::Mode3D)
                    return;
                engine::current_engine->my_renderer3d.submit_mesh(mesh, transform, camera, light, material);
            })
            .addFunction("SetClearColor", +[](float r, float g, float b, float a) {
                if (engine::current_engine == nullptr || engine::current_engine->render_mode != RenderMode::Mode3D)
                    return;
                engine::current_engine->my_renderer3d.set_clear_color(r, g, b, a);
            })
        .endNamespace()
        .beginClass<actor>("Actor")
            .addFunction("GetName", &actor::GetName)
            .addFunction("GetID", &actor::GetID)
            .addFunction("GetComponentByKey", &actor::GetComponentByKey)
            .addFunction("GetComponent", &actor::GetComponent)
            .addFunction("GetComponents", &actor::GetComponents)
            .addFunction("AddComponent", &actor::AddComponent)
            .addFunction("RemoveComponent", &actor::RemoveComponent)
        .endClass()
        .beginNamespace("Actor")
            .addFunction("Find", &scene::FindActorByName)
            .addFunction("FindAll", &scene::FindAllActorsByName)
            .addFunction("Instantiate", &scene::InstantiateActor)
            .addFunction("Destroy", &scene::DestroyActor)
        .endNamespace()
        .beginNamespace("Debug")
            .addFunction("Log", &Helper::Log)
        .endNamespace()
        .beginNamespace("Application")
            .addFunction("Quit", &Helper::Quit)
            .addFunction("Sleep", &Helper::Sleep)
            .addFunction("GetFrame", &Helper::GetFrame)
            .addFunction("GetDeltaTime", &engine::GetDeltaTime)
            .addFunction("GetScreenWidth", &engine::GetScreenWidth)
            .addFunction("GetScreenHeight", &engine::GetScreenHeight)
            .addFunction("OpenURL", &Helper::OpenURL)
        .endNamespace()
        .beginNamespace("Input")
            .addFunction("GetKey", &Input::GetKey)
            .addFunction("GetKeyDown", &Input::GetKeyDown)
            .addFunction("GetKeyUp", &Input::GetKeyUp)
            .addFunction("GetMousePosition", &Input::GetMousePosition)
            .addFunction("GetMouseDelta", &Input::GetMouseDelta)
            .addFunction("GetMouseButton", &Input::GetMouseButton)
            .addFunction("GetMouseButtonDown", &Input::GetMouseButtonDown)
            .addFunction("GetMouseButtonUp", &Input::GetMouseButtonUp)
            .addFunction("GetMouseScrollDelta", &Input::GetMouseScrollDelta)
            .addFunction("HideCursor", &Input::HideCursor)
            .addFunction("ShowCursor", &Input::ShowCursor)
            .addFunction("SetRelativeMouseMode", &Input::SetRelativeMouseMode)
        .endNamespace()
        .beginNamespace("Audio")
            .addFunction("Play", &audio::Play)
            .addFunction("Halt", &audio::Halt)
            .addFunction("SetVolume", &audio::SetVolume)
        .endNamespace()
        .beginNamespace("Physics")
            .addFunction("Raycast", &physics2d::Raycast)
            .addFunction("RaycastAll", &physics2d::RaycastAll)
        .endNamespace()
        .beginNamespace("Physics3D")
            .addFunction("OverlapBox", &physics3d::OverlapBox)
            .addFunction("Raycast", &physics3d::Raycast)
        .endNamespace()
        .beginNamespace("Camera")
            .addFunction("SetPosition", &camera2d::SetCameraPosition)
            .addFunction("GetPositionX", &camera2d::GetCameraPositionX)
            .addFunction("GetPositionY", &camera2d::GetCameraPositionY)
            .addFunction("SetZoom", &camera2d::SetCameraZoom)
            .addFunction("GetZoom", &camera2d::GetCameraZoom)
        .endNamespace()
        .beginNamespace("Scene")
            .addFunction("Load", &scene::LoadScene)
            .addFunction("GetCurrent", &scene::GetCurrentScene)
            .addFunction("DontDestroy", &scene::DontDestroyActor)
        .endNamespace()
        .beginNamespace("Event")
            .addFunction("Publish", &scene::PublishEvent)
            .addFunction("Subscribe", &scene::SubscribeEvent)
            .addFunction("Unsubscribe", &scene::UnsubscribeEvent)
        .endNamespace()
        .beginNamespace("Image")
            .addFunction("Draw", +[](const std::string& image_name, float x, float y) {
                if (engine::current_engine == nullptr || engine::current_engine->render_mode != RenderMode::Mode2D)
                    return;
                ui_system::DrawWorldImage(image_name, x, y);
            })
            .addFunction("DrawEx", +[](const std::string& image_name, float x, float y, float rotation_degrees, float scale_x, float scale_y, float pivot_x, float pivot_y, float r, float g, float b, float a, float sorting_order) {
                if (engine::current_engine == nullptr || engine::current_engine->render_mode != RenderMode::Mode2D)
                    return;
                ui_system::DrawWorldImageEx(image_name, x, y, rotation_degrees, scale_x, scale_y, pivot_x, pivot_y, r, g, b, a, sorting_order);
            })
        .endNamespace()
        .beginNamespace("UISystem")
            .addFunction("DrawImage", &ui_system::DrawImage)
            .addFunction("DrawImageEx", &ui_system::DrawImageEx)
            .addFunction("DrawText", &ui_system::DrawText)
            .addFunction("DrawPixel", &ui_system::DrawPixel)
        .endNamespace();

    Input::Init();
    
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cout << "error: SDL_Init failed: " << SDL_GetError();
        exit(0);
    }
    int img_flags = IMG_INIT_PNG;
    if ((IMG_Init(img_flags) & img_flags) != img_flags) {
        std::cout << "error: IMG_Init failed: " << IMG_GetError();
        exit(0);
    }
    if (render_mode == RenderMode::Mode3D)
        my_renderer3d.initialize(game_title, static_cast<int>(camera_w), static_cast<int>(camera_h));
    else
        my_renderer2d.initialize(game_title);

    if (!audio::Initialize()) {
        std::cout << "error: failed to initialize audio";
        exit(0);
    }
    
    my_template.load_templates();
    my_scene.load_scene(initial_scene, my_template.templates);
    game_running = 1;
}

void engine::end() {
    if (render_mode == RenderMode::Mode3D)
        my_renderer3d.end();
    else
        my_renderer2d.end();

    my_ui_system.clear();

    physics2d::Shutdown();
    physics3d::Shutdown();
    my_scene.actors.clear();
    my_scene.pending_onstart.clear();
    my_scene.pending_event_subscription_changes.clear();
    my_scene.event_subscriptions.clear();
    my_scene.loaded_component_types.clear();
    if (lua_state != nullptr) lua_close(lua_state);
    camera2d::current_camera2d = nullptr;
    current_engine = nullptr;
    audio::Shutdown();
    IMG_Quit();
    SDL_Quit();
}
