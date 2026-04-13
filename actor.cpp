#include "actor.h"
#include "particle.h"
#include "scene_DB.h"

namespace {

bool lua_refs_equal(lua_State* lua_state, const luabridge::LuaRef& left, const luabridge::LuaRef& right) {
    left.push(lua_state);
    right.push(lua_state);
    bool equal = lua_rawequal(lua_state, -1, -2) != 0;
    lua_pop(lua_state, 2);
    return equal;
}

}

component_instance::~component_instance() {
    delete native_particle_system;
    native_particle_system = nullptr;
}

std::string actor::GetName() const {
    return name;
}

int actor::GetID() const {
    return ID;
}

luabridge::LuaRef actor::GetComponentByKey(const std::string& key) const {
    for (const std::unique_ptr<component_instance>& component_ptr : component_instances) {
        const component_instance& component = *component_ptr;
        if (!component.removed && component.key == key)
            return component.self;
    }
    return luabridge::LuaRef(lua_state);
}

luabridge::LuaRef actor::GetComponent(const std::string& type_name) const {
    for (const std::unique_ptr<component_instance>& component_ptr : component_instances) {
        const component_instance& component = *component_ptr;
        if (!component.removed && component.type == type_name)
            return component.self;
    }
    return luabridge::LuaRef(lua_state);
}

luabridge::LuaRef actor::GetComponents(const std::string& type_name) const {
    luabridge::LuaRef matching_components = luabridge::newTable(lua_state);
    int index = 1;
    for (const std::unique_ptr<component_instance>& component_ptr : component_instances) {
        const component_instance& component = *component_ptr;
        if (component.removed || component.type != type_name)
            continue;
        matching_components[index] = component.self;
        index++;
    }
    return matching_components;
}

luabridge::LuaRef actor::AddComponent(const std::string& type_name) {
    if (owner_scene == nullptr)
        return luabridge::LuaRef(lua_state);

    owner_scene->load_component_type(type_name);

    actor_component component;
    component.key = "r" + std::to_string(owner_scene->runtime_component_id);
    owner_scene->runtime_component_id++;
    component.type = type_name;

    auto type_it = owner_scene->loaded_component_types.find(type_name);
    if (type_it == owner_scene->loaded_component_types.end()) {
        std::cout << "error: failed to locate component " << type_name;
        exit(0);
    }
    std::unique_ptr<component_instance> new_instance = owner_scene->create_component_instance(*this, component);

    auto insert_pos = component_instances.end();
    for (auto it = component_instances.begin(); it != component_instances.end(); ++it) {
        if ((*it)->key > component.key) {
            insert_pos = it;
            break;
        }
    }
    auto inserted = component_instances.insert(insert_pos, std::move(new_instance));
    owner_scene->pending_onstart.emplace_back(this, inserted->get());
    return (*inserted)->self;
}

void actor::RemoveComponent(const luabridge::LuaRef& component_ref) {
    if (owner_scene == nullptr)
        return;

    for (const std::unique_ptr<component_instance>& component_ptr : component_instances) {
        component_instance& component = *component_ptr;
        if (component.removed)
            continue;
        if (!lua_refs_equal(owner_scene->lua_state, component.self, component_ref))
            continue;
        owner_scene->pending_component_removal.emplace_back(this, &component);
        component.removed = true;
        component.self["enabled"] = false;
        return;
    }
}
