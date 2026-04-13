#ifndef SCENE_DB_H
#define SCENE_DB_H
#include "glm.hpp"
#include <memory>
#include <vector>
#include <filesystem>
#include <iostream>
#include <cmath>
#include "actor.h"
#include "lua.hpp"
#include "LuaBridge/LuaBridge.h"
#include "document.h"
#include "filereadstream.h"
#include <cstddef>
#include <unordered_map>
#include <memory>

class scene
{
public:
    friend class engine;
    friend struct actor;

    std::vector<std::unique_ptr<actor>> actors;
    std::vector<actor*> pending_actor_destroy;
    struct pending_component_start {
        actor* owner = nullptr;
        component_instance* component = nullptr;

        pending_component_start(actor* in_owner, component_instance* in_component)
            : owner(in_owner), component(in_component) {}
    };
    std::vector<pending_component_start> pending_onstart;
    struct pending_component_remove {
        actor* owner = nullptr;
        component_instance* component = nullptr;

        pending_component_remove(actor* in_owner, component_instance* in_component)
            : owner(in_owner), component(in_component) {}
    };
    std::vector<pending_component_remove> pending_component_removal;
    struct event_subscription {
        luabridge::LuaRef component;
        luabridge::LuaRef function;

        event_subscription(const luabridge::LuaRef& in_component,
                           const luabridge::LuaRef& in_function)
            : component(in_component), function(in_function) {}
    };
    struct pending_event_subscription_change {
        bool subscribe = true;
        std::string event_type = "";
        luabridge::LuaRef component;
        luabridge::LuaRef function;

        pending_event_subscription_change(bool in_subscribe,
                                          const std::string& in_event_type,
                                          const luabridge::LuaRef& in_component,
                                          const luabridge::LuaRef& in_function)
            : subscribe(in_subscribe),
              event_type(in_event_type),
              component(in_component),
              function(in_function) {}
    };
    std::unordered_map<std::string, std::vector<event_subscription>> event_subscriptions;
    std::vector<pending_event_subscription_change> pending_event_subscription_changes;
    struct component_type_info {
        luabridge::LuaRef base_table;
        std::optional<luabridge::LuaRef> on_start;
        std::optional<luabridge::LuaRef> on_update;
        std::optional<luabridge::LuaRef> on_late_update;
        std::optional<luabridge::LuaRef> on_destroy;
        std::optional<luabridge::LuaRef> on_collision_enter;
        std::optional<luabridge::LuaRef> on_collision_exit;
        std::optional<luabridge::LuaRef> on_trigger_enter;
        std::optional<luabridge::LuaRef> on_trigger_exit;

        component_type_info(luabridge::LuaRef&& in_base_table)
            : base_table(std::move(in_base_table)) {}
    };

    void load_scene(std::string& scene_name, std::unordered_map<std::string, actor>& templates);
    void process_onstart();
    void process_onupdate();
    void process_onlateupdate();
    void process_ondestroy();
    void process_event_subscriptions_changes();
    static void handle_contact(actor* owner, actor* other, const b2Vec2& point, const b2Vec2& relative_velocity, const b2Vec2& normal, bool is_trigger, bool is_enter);
    static void handle_contact3d(actor* owner, actor* other, const glm::vec3& point, const glm::vec3& relative_velocity, const glm::vec3& normal, bool is_enter);
    void cleanup_end_of_frame();

    std::unordered_map<std::string, component_type_info> loaded_component_types;
    int runtime_component_id = 0;
    void load_component_type(const std::string& component_type);
    void instantiate_components(actor& a);
    std::unique_ptr<component_instance> create_component_instance(actor& a, const actor_component& component);
    std::unordered_map<std::string, actor>* templates = nullptr;
    lua_State* lua_state = nullptr;
    inline static scene* current_scene = nullptr;
    std::string current_scene_name = "";
    std::string new_scene_name = "";
    bool has_new_scene = false;
    int next_actor_id = 0;
    
    static luabridge::LuaRef FindActorByName(const std::string& name); // api
    static luabridge::LuaRef FindAllActorsByName(const std::string& name); // api
    static luabridge::LuaRef InstantiateActor(const std::string& template_name); // api
    static void DestroyActor(actor* actor_to_destroy); // api
    static void LoadScene(const std::string& scene_name); // api
    static std::string GetCurrentScene(); // api
    static void DontDestroyActor(actor* actor_to_preserve); // api
    static void PublishEvent(const std::string& event_type, const luabridge::LuaRef& event_object); // api
    static void SubscribeEvent(const std::string& event_type, const luabridge::LuaRef& component, const luabridge::LuaRef& function); // api
    static void UnsubscribeEvent(const std::string& event_type, const luabridge::LuaRef& component, const luabridge::LuaRef& function); // api
};
#endif // !SCENE_DB_H
