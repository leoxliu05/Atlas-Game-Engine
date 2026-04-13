#ifndef ACTOR_H
#define ACTOR_H
#include "lua.hpp"
#include "LuaBridge/LuaBridge.h"
#include "glm.hpp"
#include "physics2d.h"
#include "physics3d.h"
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

class scene;
class ParticleSystem;

struct actor_component {
    std::string key = "";
    std::string type = "";
    std::unordered_map<std::string, std::variant<std::string, std::int64_t, double, bool>> properties;
};

struct component_instance {
    std::string key = "";
    std::string type = "";
    luabridge::LuaRef self;
    std::unique_ptr<Rigidbody> native_rigidbody;
    std::unique_ptr<Rigidbody3D> native_rigidbody3d;
    ParticleSystem* native_particle_system = nullptr;
    std::optional<luabridge::LuaRef> on_start;
    std::optional<luabridge::LuaRef> on_update;
    std::optional<luabridge::LuaRef> on_late_update;
    std::optional<luabridge::LuaRef> on_destroy;
    std::optional<luabridge::LuaRef> on_collision_enter;
    std::optional<luabridge::LuaRef> on_collision_exit;
    std::optional<luabridge::LuaRef> on_trigger_enter;
    std::optional<luabridge::LuaRef> on_trigger_exit;
    bool on_start_called = false;
    bool on_destroy_called = false;
    bool pending_start = true;
    bool removed = false;

    ~component_instance();

    component_instance(const std::string& in_key,
                       const std::string& in_type,
                       luabridge::LuaRef&& in_self,
                       std::unique_ptr<Rigidbody>&& in_native_rigidbody = nullptr,
                       std::unique_ptr<Rigidbody3D>&& in_native_rigidbody3d = nullptr,
                       ParticleSystem* in_native_particle_system = nullptr)
        : key(in_key),
          type(in_type),
          self(std::move(in_self)),
          native_rigidbody(std::move(in_native_rigidbody)),
          native_rigidbody3d(std::move(in_native_rigidbody3d)),
          native_particle_system(in_native_particle_system) {}
};

struct actor
{
public:
    std::string name = "";
    int ID = -1;
    lua_State* lua_state = nullptr;
    scene* owner_scene = nullptr;
    bool pending_destroy = false;
    bool dont_destroy = false;
    std::vector<actor_component> components;
    std::vector<std::unique_ptr<component_instance>> component_instances;
    
    std::string GetName() const; // api
    int GetID() const; // api
    luabridge::LuaRef GetComponentByKey(const std::string& key) const; // api
    luabridge::LuaRef GetComponent(const std::string& type_name) const; // api
    luabridge::LuaRef GetComponents(const std::string& type_name) const; // api
    luabridge::LuaRef AddComponent(const std::string& type_name); // api
    void RemoveComponent(const luabridge::LuaRef& component_ref); // api

    actor() = default;

    actor(const actor& other)
        : name(other.name),
          ID(other.ID),
          lua_state(other.lua_state),
          owner_scene(other.owner_scene),
          pending_destroy(other.pending_destroy),
          dont_destroy(other.dont_destroy),
          components(other.components) {}

    actor& operator=(const actor& other) {
        if (this == &other)
            return *this;

        name = other.name;
        ID = other.ID;
        lua_state = other.lua_state;
        owner_scene = other.owner_scene;
        pending_destroy = other.pending_destroy;
        dont_destroy = other.dont_destroy;
        components = other.components;
        component_instances.clear();
        return *this;
    }

    actor(actor&& other) = default;
    actor& operator=(actor&& other) = default;
};

#endif // !ACTOR_H
