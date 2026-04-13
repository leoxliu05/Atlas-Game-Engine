#include "scene_DB.h"
#include "actor_template_DB.h"
#include "particle.h"
#include <algorithm>
#include <iostream>

namespace {

bool lua_refs_equal(lua_State* lua_state, const luabridge::LuaRef& left, const luabridge::LuaRef& right) {
    left.push(lua_state);
    right.push(lua_state);
    bool equal = lua_rawequal(lua_state, -1, -2) != 0;
    lua_pop(lua_state, 2);
    return equal;
}

bool component_is_enabled(lua_State* lua_state, const component_instance& component) {
    component.self.push(lua_state);
    lua_getfield(lua_state, -1, "enabled");
    bool enabled = true;
    if (lua_isboolean(lua_state, -1))
        enabled = lua_toboolean(lua_state, -1) != 0;
    lua_pop(lua_state, 2);
    return enabled;
}

void report_error(const std::string& actor_name, const luabridge::LuaException& e) {
    std::string error_message = e.what();
    std::replace(error_message.begin(), error_message.end(), '\\', '/');
    std::cout << "\033[31m" << actor_name << " : " << error_message << "\033[0m" << std::endl;
}

component_instance* find_component_instance(actor*& owner, const luabridge::LuaRef& component_ref) {
    owner = nullptr;
    if (scene::current_scene == nullptr)
        return nullptr;

    for (const std::unique_ptr<actor>& actor_ptr : scene::current_scene->actors) {
        if (actor_ptr == nullptr)
            continue;
        for (const std::unique_ptr<component_instance>& component_ptr : actor_ptr->component_instances) {
            if (component_ptr == nullptr)
                continue;
            if (!lua_refs_equal(scene::current_scene->lua_state, component_ptr->self, component_ref))
                continue;
            owner = actor_ptr.get();
            return component_ptr.get();
        }
    }

    return nullptr;
}

void remove_event_subscriptions_for_component(scene& current_scene, const luabridge::LuaRef& component_ref) {
    for (auto subscriptions_it = current_scene.event_subscriptions.begin(); subscriptions_it != current_scene.event_subscriptions.end(); ) {
        std::vector<scene::event_subscription>& subscriptions = subscriptions_it->second;
        subscriptions.erase(
            std::remove_if(
                subscriptions.begin(),
                subscriptions.end(),
                [&](const scene::event_subscription& subscription) {
                    return lua_refs_equal(current_scene.lua_state, subscription.component, component_ref);
                }
            ),
            subscriptions.end()
        );

        if (subscriptions.empty())
            subscriptions_it = current_scene.event_subscriptions.erase(subscriptions_it);
        else
            ++subscriptions_it;
    }

    current_scene.pending_event_subscription_changes.erase(
        std::remove_if(
            current_scene.pending_event_subscription_changes.begin(),
            current_scene.pending_event_subscription_changes.end(),
            [&](const scene::pending_event_subscription_change& change) {
                return lua_refs_equal(current_scene.lua_state, change.component, component_ref);
            }
        ),
        current_scene.pending_event_subscription_changes.end()
    );
}

void call_component_lifecycle(actor& owner,
                              component_instance& component,
                              const std::optional<luabridge::LuaRef>& lifecycle_function) {
    if (!lifecycle_function.has_value())
        return;
    if (owner.pending_destroy || component.removed || !component_is_enabled(owner.lua_state, component))
        return;

    try {
        lifecycle_function.value()(component.self);
    } catch (const luabridge::LuaException& e) {
        report_error(owner.name, e);
    }
}

void call_ondestroy(actor& owner, component_instance& component) {
    if (component.on_destroy_called)
        return;

    if (component.native_rigidbody != nullptr) {
        component.native_rigidbody->OnDestroy();
    } else if (component.native_rigidbody3d != nullptr) {
        component.native_rigidbody3d->OnDestroy();
    } else if (component.native_particle_system != nullptr) {
        component.native_particle_system->OnDestroy();
    } else if (component.on_destroy.has_value()) {
        try {
            component.on_destroy.value()(component.self);
        } catch (const luabridge::LuaException& e) {
            report_error(owner.name, e);
        }
    }

    component.on_destroy_called = true;
}

void call_component_contact_lifecycle(actor& owner,
                                      component_instance& component,
                                      const std::optional<luabridge::LuaRef>& lifecycle_function,
                                      actor* other_actor,
                                      const b2Vec2& point,
                                      const b2Vec2& relative_velocity,
                                      const b2Vec2& normal) {
    if (!lifecycle_function.has_value() || component.removed || component.pending_start || !component_is_enabled(owner.lua_state, component))
        return;

    collision contact_data;
    contact_data.other = other_actor;
    contact_data.point = point;
    contact_data.relative_velocity = relative_velocity;
    contact_data.normal = normal;

    try {
        lifecycle_function.value()(component.self, &contact_data);
    } catch (const luabridge::LuaException& e) {
        report_error(owner.name, e);
    }
}

void call_component_contact_lifecycle3d(actor& owner,
                                        component_instance& component,
                                        const std::optional<luabridge::LuaRef>& lifecycle_function,
                                        actor* other_actor,
                                        const glm::vec3& point,
                                        const glm::vec3& relative_velocity,
                                        const glm::vec3& normal) {
    if (!lifecycle_function.has_value() || component.removed || component.pending_start || !component_is_enabled(owner.lua_state, component))
        return;

    collision3d contact_data;
    contact_data.other = other_actor;
    contact_data.point = point;
    contact_data.relative_velocity = relative_velocity;
    contact_data.normal = normal;

    try {
        lifecycle_function.value()(component.self, &contact_data);
    } catch (const luabridge::LuaException& e) {
        report_error(owner.name, e);
    }
}

}

luabridge::LuaRef scene::FindActorByName(const std::string& name) {
    for (const std::unique_ptr<actor>& actor_ptr : current_scene->actors) {
        actor& a = *actor_ptr;
        if (a.pending_destroy)
            continue;
        if (a.name == name)
            return luabridge::LuaRef(current_scene->lua_state, &a);
    }
    return luabridge::LuaRef(current_scene->lua_state);
}

luabridge::LuaRef scene::FindAllActorsByName(const std::string& name) {
    luabridge::LuaRef matching_actors = luabridge::newTable(current_scene->lua_state);
    int index = 1;
    for (const std::unique_ptr<actor>& actor_ptr : current_scene->actors) {
        actor& a = *actor_ptr;
        if (a.pending_destroy)
            continue;
        if (a.name != name)
            continue;
        matching_actors[index] = &a;
        index++;
    }
    return matching_actors;
}

luabridge::LuaRef scene::InstantiateActor(const std::string& template_name) {
    if (current_scene->templates == nullptr) {
        std::cout << "error: template " << template_name << " is missing";
        exit(0);
    }

    auto it = current_scene->templates->find(template_name);
    if (it == current_scene->templates->end()) {
        std::cout << "error: template " << template_name << " is missing";
        exit(0);
    }

    actor a = it->second;
    a.component_instances.clear();
    a.lua_state = current_scene->lua_state;
    a.owner_scene = current_scene;
    a.pending_destroy = false;
    a.ID = current_scene->next_actor_id;
    current_scene->next_actor_id++;
    for (const actor_component& component : a.components)
        current_scene->load_component_type(component.type);
    current_scene->actors.emplace_back(std::make_unique<actor>(std::move(a)));
    actor& created_actor = *current_scene->actors.back();
    current_scene->instantiate_components(created_actor);
    return luabridge::LuaRef(current_scene->lua_state, &created_actor);
}

void scene::DestroyActor(actor* actor_to_destroy) {
    if (actor_to_destroy == nullptr || actor_to_destroy->pending_destroy)
        return;

    actor_to_destroy->pending_destroy = true;
    current_scene->pending_actor_destroy.emplace_back(actor_to_destroy);
    for (const std::unique_ptr<component_instance>& component_ptr : actor_to_destroy->component_instances) {
        component_instance& component = *component_ptr;
        if (!component.removed)
            current_scene->pending_component_removal.emplace_back(actor_to_destroy, &component);
        component.removed = true;
        component.self["enabled"] = false;
    }
}

void scene::LoadScene(const std::string& scene_name) {
    current_scene->new_scene_name = scene_name;
    current_scene->has_new_scene = true;
}

std::string scene::GetCurrentScene() {
    return current_scene->current_scene_name;
}

void scene::PublishEvent(const std::string& event_type, const luabridge::LuaRef& event_object) {
    if (current_scene == nullptr)
        return;

    auto subscriptions_it = current_scene->event_subscriptions.find(event_type);
    if (subscriptions_it == current_scene->event_subscriptions.end())
        return;

    for (const event_subscription& subscription : subscriptions_it->second) {
        actor* owner = nullptr;
        component_instance* component = find_component_instance(owner, subscription.component);
        if (owner == nullptr || component == nullptr || owner->pending_destroy || component->removed)
            continue;

        try {
            subscription.function(subscription.component, event_object);
        } catch (const luabridge::LuaException& e) {
            report_error(owner->name, e);
        }
    }
}

void scene::SubscribeEvent(const std::string& event_type, const luabridge::LuaRef& component, const luabridge::LuaRef& function) {
    if (current_scene == nullptr)
        return;
    current_scene->pending_event_subscription_changes.emplace_back(true, event_type, component, function);
}

void scene::UnsubscribeEvent(const std::string& event_type, const luabridge::LuaRef& component, const luabridge::LuaRef& function) {
    if (current_scene == nullptr)
        return;
    current_scene->pending_event_subscription_changes.emplace_back(false, event_type, component, function);
}

void scene::DontDestroyActor(actor* actor_to_preserve) {
    if (actor_to_preserve == nullptr)
        return;
    actor_to_preserve->dont_destroy = true;
}

void scene::load_scene(std::string& scene_name, std::unordered_map<std::string, actor>& templates_map) {
    const bool is_initial_load = current_scene_name.empty() && actors.empty() && templates == nullptr;

    current_scene = this;
    templates = &templates_map;

    if (is_initial_load) {
        actors.clear();
        next_actor_id = 0;
    } else {
        for (const std::unique_ptr<actor>& actor_ptr : actors) {
            if (actor_ptr == nullptr || actor_ptr->dont_destroy)
                continue;
            for (const std::unique_ptr<component_instance>& component_ptr : actor_ptr->component_instances) {
                call_ondestroy(*actor_ptr, *component_ptr);
                remove_event_subscriptions_for_component(*this, component_ptr->self);
            }
        }

        std::vector<std::unique_ptr<actor>> preserved_actors;
        preserved_actors.reserve(actors.size());
        for (auto& actor_ptr : actors) {
            if (!actor_ptr->dont_destroy)
                continue;
            actor_ptr->owner_scene = this;
            actor_ptr->pending_destroy = false;
            preserved_actors.emplace_back(std::move(actor_ptr));
        }
        actors = std::move(preserved_actors);
    }

    pending_onstart.clear();
    pending_actor_destroy.clear();
    pending_component_removal.clear();
    current_scene_name = scene_name;

    std::string config_path_scene = "resources/scenes/" + scene_name + ".scene";
    if (!std::filesystem::exists(config_path_scene)) {
        std::cout << "error: scene " + scene_name + " is missing";
        exit(0);
    }

    rapidjson::Document json_scene;
    Helper::ReadJsonFile(config_path_scene, json_scene);
    std::vector<actor*> newly_loaded_actors;
    actors.reserve(actors.size() + json_scene["actors"].Size());
    newly_loaded_actors.reserve(json_scene["actors"].Size());
    for (const auto& entry : json_scene["actors"].GetArray()) {
        actor a;
        if (entry.HasMember("template")) {
            std::string tname = entry["template"].GetString();
            if (!tname.empty()) {
                auto it = templates->find(tname);
                if (it == templates->end()) {
                    std::cout << "error: template " << tname << " is missing";
                    exit(0);
                }
                a = it->second;
                a.component_instances.clear();
            }
        }

        a.ID = next_actor_id;
        next_actor_id++;
        a.lua_state = lua_state;
        a.owner_scene = this;
        a.pending_destroy = false;
        a.dont_destroy = false;
        if (entry.HasMember("name"))
            a.name = entry["name"].GetString();
        ReadActorComponents(entry, a);
        for (const actor_component& component : a.components)
            load_component_type(component.type);

        actors.emplace_back(std::make_unique<actor>(std::move(a)));
        newly_loaded_actors.emplace_back(actors.back().get());
    }

    for (actor* actor_ptr : newly_loaded_actors)
        instantiate_components(*actor_ptr);
}

void scene::load_component_type(const std::string& component_type) {
    if (loaded_component_types.find(component_type) != loaded_component_types.end())
        return;

    if (component_type == "Rigidbody" || component_type == "Rigidbody3D" || component_type == "ParticleSystem") {
        loaded_component_types.emplace(component_type, component_type_info(luabridge::LuaRef(lua_state)));
        return;
    }

    std::string component_path = "resources/component_types/" + component_type + ".lua";
    if (!std::filesystem::exists(component_path)) {
        std::cout << "error: failed to locate component " << component_type;
        exit(0);
    }

    if (luaL_dofile(lua_state, component_path.c_str()) != LUA_OK) {
        const char* lua_error = lua_tostring(lua_state, -1);
        std::cout << "problem with lua file " << std::filesystem::path(component_path).stem().string();
        if (lua_error != nullptr)
            std::cout << ": " << lua_error;
        exit(0);
    }

    luabridge::LuaRef base_table = luabridge::getGlobal(lua_state, component_type.c_str());
    auto [it, inserted] = loaded_component_types.emplace(component_type, component_type_info(std::move(base_table)));
    luabridge::LuaRef on_start = it->second.base_table["OnStart"];
    if (on_start.isFunction())
        it->second.on_start = on_start;
    luabridge::LuaRef on_update = it->second.base_table["OnUpdate"];
    if (on_update.isFunction())
        it->second.on_update = on_update;
    luabridge::LuaRef on_late_update = it->second.base_table["OnLateUpdate"];
    if (on_late_update.isFunction())
        it->second.on_late_update = on_late_update;
    luabridge::LuaRef on_destroy = it->second.base_table["OnDestroy"];
    if (on_destroy.isFunction())
        it->second.on_destroy = on_destroy;
    luabridge::LuaRef on_collision_enter = it->second.base_table["OnCollisionEnter"];
    if (on_collision_enter.isFunction())
        it->second.on_collision_enter = on_collision_enter;
    luabridge::LuaRef on_collision_exit = it->second.base_table["OnCollisionExit"];
    if (on_collision_exit.isFunction())
        it->second.on_collision_exit = on_collision_exit;
    luabridge::LuaRef on_trigger_enter = it->second.base_table["OnTriggerEnter"];
    if (on_trigger_enter.isFunction())
        it->second.on_trigger_enter = on_trigger_enter;
    luabridge::LuaRef on_trigger_exit = it->second.base_table["OnTriggerExit"];
    if (on_trigger_exit.isFunction())
        it->second.on_trigger_exit = on_trigger_exit;
}

std::unique_ptr<component_instance> scene::create_component_instance(actor& a, const actor_component& component) {
    auto type_it = loaded_component_types.find(component.type);
    if (type_it == loaded_component_types.end()) {
        std::cout << "error: failed to locate component " << component.type;
        exit(0);
    }

    if (component.type == "Rigidbody") {
        std::unique_ptr<Rigidbody> rigidbody = std::make_unique<Rigidbody>();
        rigidbody->owner = &a;
        rigidbody->key = component.key;
        for (const auto& [property_name, property_value] : component.properties) {
            if (property_name == "enabled" && std::holds_alternative<bool>(property_value)) {
                rigidbody->enabled = std::get<bool>(property_value);
                continue;
            }
            if (property_name == "body_type" && std::holds_alternative<std::string>(property_value)) {
                rigidbody->body_type = std::get<std::string>(property_value);
                continue;
            }
            if (property_name == "collider_type" && std::holds_alternative<std::string>(property_value)) {
                rigidbody->collider_type = std::get<std::string>(property_value);
                continue;
            }
            if (property_name == "trigger_type" && std::holds_alternative<std::string>(property_value)) {
                rigidbody->trigger_type = std::get<std::string>(property_value);
                continue;
            }
            if (property_name == "precise" && std::holds_alternative<bool>(property_value)) {
                rigidbody->precise = std::get<bool>(property_value);
                continue;
            }
            if (property_name == "has_collider" && std::holds_alternative<bool>(property_value)) {
                rigidbody->has_collider = std::get<bool>(property_value);
                continue;
            }
            if (property_name == "has_trigger" && std::holds_alternative<bool>(property_value)) {
                rigidbody->has_trigger = std::get<bool>(property_value);
                continue;
            }

            std::optional<float> number;
            if (std::holds_alternative<std::int64_t>(property_value))
                number = static_cast<float>(std::get<std::int64_t>(property_value));
            else if (std::holds_alternative<double>(property_value))
                number = static_cast<float>(std::get<double>(property_value));

            if (!number.has_value())
                continue;

            if (property_name == "x")
                rigidbody->x = *number;
            else if (property_name == "y")
                rigidbody->y = *number;
            else if (property_name == "rotation")
                rigidbody->rotation = *number;
            else if (property_name == "gravity_scale")
                rigidbody->gravity_scale = *number;
            else if (property_name == "density")
                rigidbody->density = *number;
            else if (property_name == "angular_friction")
                rigidbody->angular_friction = *number;
            else if (property_name == "width")
                rigidbody->width = *number;
            else if (property_name == "height")
                rigidbody->height = *number;
            else if (property_name == "radius")
                rigidbody->radius = *number;
            else if (property_name == "friction")
                rigidbody->friction = *number;
            else if (property_name == "bounciness")
                rigidbody->bounciness = *number;
            else if (property_name == "trigger_width")
                rigidbody->trigger_width = *number;
            else if (property_name == "trigger_height")
                rigidbody->trigger_height = *number;
            else if (property_name == "trigger_radius")
                rigidbody->trigger_radius = *number;
        }

        luabridge::LuaRef instance_ref(lua_state, rigidbody.get());
        return std::make_unique<component_instance>(
            component.key,
            component.type,
            std::move(instance_ref),
            std::move(rigidbody)
        );
    }

    if (component.type == "Rigidbody3D") {
        std::unique_ptr<Rigidbody3D> rigidbody = std::make_unique<Rigidbody3D>();
        rigidbody->owner = &a;
        rigidbody->key = component.key;
        for (const auto& [property_name, property_value] : component.properties) {
            if (property_name == "enabled" && std::holds_alternative<bool>(property_value)) {
                rigidbody->enabled = std::get<bool>(property_value);
                continue;
            }
            if (property_name == "body_type" && std::holds_alternative<std::string>(property_value)) {
                rigidbody->body_type = std::get<std::string>(property_value);
                continue;
            }
            if (property_name == "collider_type" && std::holds_alternative<std::string>(property_value)) {
                rigidbody->collider_type = std::get<std::string>(property_value);
                continue;
            }
            if (property_name == "precise" && std::holds_alternative<bool>(property_value)) {
                rigidbody->precise = std::get<bool>(property_value);
                continue;
            }

            std::optional<float> number;
            if (std::holds_alternative<std::int64_t>(property_value))
                number = static_cast<float>(std::get<std::int64_t>(property_value));
            else if (std::holds_alternative<double>(property_value))
                number = static_cast<float>(std::get<double>(property_value));

            if (!number.has_value())
                continue;

            if (property_name == "x")
                rigidbody->position.x = *number;
            else if (property_name == "y")
                rigidbody->position.y = *number;
            else if (property_name == "z")
                rigidbody->position.z = *number;
            else if (property_name == "rotation_x")
                rigidbody->rotation_degrees.x = *number;
            else if (property_name == "rotation_y")
                rigidbody->rotation_degrees.y = *number;
            else if (property_name == "rotation_z")
                rigidbody->rotation_degrees.z = *number;
            else if (property_name == "mass")
                rigidbody->mass = *number;
            else if (property_name == "gravity_scale")
                rigidbody->gravity_scale = *number;
            else if (property_name == "linear_damping")
                rigidbody->linear_damping = *number;
            else if (property_name == "angular_damping")
                rigidbody->angular_damping = *number;
            else if (property_name == "width")
                rigidbody->width = *number;
            else if (property_name == "height")
                rigidbody->height = *number;
            else if (property_name == "depth")
                rigidbody->depth = *number;
            else if (property_name == "radius")
                rigidbody->radius = *number;
            else if (property_name == "friction")
                rigidbody->friction = *number;
            else if (property_name == "bounciness")
                rigidbody->bounciness = *number;
        }

        luabridge::LuaRef instance_ref(lua_state, rigidbody.get());
        return std::make_unique<component_instance>(
            component.key,
            component.type,
            std::move(instance_ref),
            nullptr,
            std::move(rigidbody)
        );
    }

    if (component.type == "ParticleSystem") {
        ParticleSystem* particle_system = new ParticleSystem();
        particle_system->owner = &a;
        particle_system->key = component.key;

        auto read_float = [](const std::variant<std::string, std::int64_t, double, bool>& value) -> std::optional<float> {
            if (std::holds_alternative<std::int64_t>(value))
                return static_cast<float>(std::get<std::int64_t>(value));
            if (std::holds_alternative<double>(value))
                return static_cast<float>(std::get<double>(value));
            return std::nullopt;
        };

        auto read_int = [](const std::variant<std::string, std::int64_t, double, bool>& value) -> std::optional<int> {
            if (std::holds_alternative<std::int64_t>(value))
                return static_cast<int>(std::get<std::int64_t>(value));
            if (std::holds_alternative<double>(value))
                return static_cast<int>(std::get<double>(value));
            return std::nullopt;
        };

        for (const auto& [property_name, property_value] : component.properties) {
            if (property_name == "enabled" && std::holds_alternative<bool>(property_value)) {
                particle_system->enabled = std::get<bool>(property_value);
                continue;
            }

            if (property_name == "image" && std::holds_alternative<std::string>(property_value)) {
                particle_system->image = std::get<std::string>(property_value);
                continue;
            }

            std::optional<float> float_value = read_float(property_value);
            if (float_value.has_value()) {
                if (property_name == "x")
                    particle_system->x = *float_value;
                else if (property_name == "y")
                    particle_system->y = *float_value;
                else if (property_name == "emit_angle_min")
                    particle_system->emit_angle_min = *float_value;
                else if (property_name == "emit_angle_max")
                    particle_system->emit_angle_max = *float_value;
                else if (property_name == "emit_radius_min")
                    particle_system->emit_radius_min = *float_value;
                else if (property_name == "emit_radius_max")
                    particle_system->emit_radius_max = *float_value;
                else if (property_name == "start_scale_min")
                    particle_system->start_scale_min = *float_value;
                else if (property_name == "start_scale_max")
                    particle_system->start_scale_max = *float_value;
                else if (property_name == "start_speed_min")
                    particle_system->start_speed_min = *float_value;
                else if (property_name == "start_speed_max")
                    particle_system->start_speed_max = *float_value;
                else if (property_name == "rotation_min")
                    particle_system->rotation_min = *float_value;
                else if (property_name == "rotation_max")
                    particle_system->rotation_max = *float_value;
                else if (property_name == "rotation_speed_min")
                    particle_system->rotation_speed_min = *float_value;
                else if (property_name == "rotation_speed_max")
                    particle_system->rotation_speed_max = *float_value;
                else if (property_name == "gravity_scale_x")
                    particle_system->gravity_scale_x = *float_value;
                else if (property_name == "gravity_scale_y")
                    particle_system->gravity_scale_y = *float_value;
                else if (property_name == "drag_factor")
                    particle_system->drag_factor = *float_value;
                else if (property_name == "angular_drag_factor")
                    particle_system->angular_drag_factor = *float_value;
                else if (property_name == "end_scale")
                    particle_system->end_scale = *float_value;
            }

            std::optional<int> int_value = read_int(property_value);
            if (!int_value.has_value())
                continue;

            if (property_name == "frames_between_bursts")
                particle_system->frames_between_bursts = std::max(1, *int_value);
            else if (property_name == "burst_quantity")
                particle_system->burst_quantity = std::max(1, *int_value);
            else if (property_name == "duration_frames")
                particle_system->duration_frames = std::max(1, *int_value);
            else if (property_name == "sorting_order")
                particle_system->sorting_order = *int_value;
            else if (property_name == "start_color_r")
                particle_system->start_color_r = std::clamp(*int_value, 0, 255);
            else if (property_name == "start_color_g")
                particle_system->start_color_g = std::clamp(*int_value, 0, 255);
            else if (property_name == "start_color_b")
                particle_system->start_color_b = std::clamp(*int_value, 0, 255);
            else if (property_name == "start_color_a")
                particle_system->start_color_a = std::clamp(*int_value, 0, 255);
            else if (property_name == "end_color_r")
                particle_system->end_color_r = std::clamp(*int_value, 0, 255);
            else if (property_name == "end_color_g")
                particle_system->end_color_g = std::clamp(*int_value, 0, 255);
            else if (property_name == "end_color_b")
                particle_system->end_color_b = std::clamp(*int_value, 0, 255);
            else if (property_name == "end_color_a")
                particle_system->end_color_a = std::clamp(*int_value, 0, 255);
        }

        luabridge::LuaRef instance_ref(lua_state, particle_system);
        return std::make_unique<component_instance>(
            component.key,
            component.type,
            std::move(instance_ref),
            nullptr,
            nullptr,
            particle_system
        );
    }

    luabridge::LuaRef instance_table = luabridge::newTable(lua_state);
    for (const auto& [property_name, property_value] : component.properties) {
        if (std::holds_alternative<std::string>(property_value))
            instance_table[property_name] = std::get<std::string>(property_value);
        else if (std::holds_alternative<std::int64_t>(property_value))
            instance_table[property_name] = std::get<std::int64_t>(property_value);
        else if (std::holds_alternative<double>(property_value))
            instance_table[property_name] = std::get<double>(property_value);
        else if (std::holds_alternative<bool>(property_value))
            instance_table[property_name] = std::get<bool>(property_value);
    }
    instance_table["actor"] = &a;
    instance_table["key"] = component.key;
    instance_table["enabled"] = true;
    luabridge::LuaRef new_metatable = luabridge::newTable(lua_state);
    new_metatable["__index"] = type_it->second.base_table;

    instance_table.push(lua_state);
    new_metatable.push(lua_state);
    lua_setmetatable(lua_state, -2);
    lua_pop(lua_state, 1);

    auto instance = std::make_unique<component_instance>(component.key, component.type, std::move(instance_table));
    instance->on_start = type_it->second.on_start;
    instance->on_update = type_it->second.on_update;
    instance->on_late_update = type_it->second.on_late_update;
    instance->on_destroy = type_it->second.on_destroy;
    instance->on_collision_enter = type_it->second.on_collision_enter;
    instance->on_collision_exit = type_it->second.on_collision_exit;
    instance->on_trigger_enter = type_it->second.on_trigger_enter;
    instance->on_trigger_exit = type_it->second.on_trigger_exit;
    return instance;
}

void scene::instantiate_components(actor& a) {
    a.component_instances.clear();
    a.component_instances.reserve(a.components.size());
    for (const actor_component& component : a.components) {
        a.component_instances.emplace_back(create_component_instance(a, component));
        pending_onstart.emplace_back(&a, a.component_instances.back().get());
    }
}

void scene::process_onstart() {
    if (pending_onstart.empty())
        return;

    std::vector<pending_component_start> current_pending;
    current_pending.swap(pending_onstart);

    for (const pending_component_start& pending_component : current_pending) {
        if (pending_component.owner == nullptr || pending_component.component == nullptr)
            continue;

        component_instance& component = *pending_component.component;
        if (pending_component.owner->pending_destroy || component.removed || component.on_start_called)
            continue;

        const bool enabled = component_is_enabled(lua_state, component);
        if (enabled && component.native_rigidbody != nullptr)
            component.native_rigidbody->OnStart();
        else if (enabled && component.native_rigidbody3d != nullptr)
            component.native_rigidbody3d->OnStart();
        else if (enabled && component.native_particle_system != nullptr)
            component.native_particle_system->OnStart();
        else if (component.on_start.has_value() && enabled)
            call_component_lifecycle(*pending_component.owner, component, component.on_start);
        component.on_start_called = true;
        component.pending_start = false;
    }
}

void scene::process_onupdate() {
    std::vector<component_instance*> components_snapshot;
    const std::size_t actor_count = actors.size();
    for (std::size_t actor_index = 0; actor_index < actor_count; ++actor_index) {
        actor* actor_ptr = actors[actor_index].get();
        if (actor_ptr == nullptr)
            continue;
        actor& a = *actor_ptr;
        if (a.pending_destroy)
            continue;

        components_snapshot.clear();
        components_snapshot.reserve(a.component_instances.size());
        for (const std::unique_ptr<component_instance>& component_ptr : a.component_instances)
            components_snapshot.emplace_back(component_ptr.get());

        for (component_instance* component_ptr : components_snapshot) {
            if (component_ptr == nullptr || component_ptr->pending_start)
                continue;
            if (component_ptr->native_particle_system != nullptr) {
                if (component_is_enabled(lua_state, *component_ptr))
                    component_ptr->native_particle_system->OnUpdate();
                continue;
            }
            if (!component_ptr->on_update.has_value())
                continue;
            call_component_lifecycle(a, *component_ptr, component_ptr->on_update);
        }
    }
}

void scene::process_onlateupdate() {
    std::vector<component_instance*> components_snapshot;
    const std::size_t actor_count = actors.size();
    for (std::size_t actor_index = 0; actor_index < actor_count; ++actor_index) {
        actor* actor_ptr = actors[actor_index].get();
        if (actor_ptr == nullptr)
            continue;
        actor& a = *actor_ptr;
        if (a.pending_destroy)
            continue;

        components_snapshot.clear();
        components_snapshot.reserve(a.component_instances.size());
        for (const std::unique_ptr<component_instance>& component_ptr : a.component_instances)
            components_snapshot.emplace_back(component_ptr.get());

        for (component_instance* component_ptr : components_snapshot) {
            if (component_ptr == nullptr || component_ptr->pending_start || !component_ptr->on_late_update.has_value())
                continue;
            call_component_lifecycle(a, *component_ptr, component_ptr->on_late_update);
        }
    }
}

void scene::process_ondestroy() {
    if (pending_component_removal.empty())
        return;

    std::vector<pending_component_remove> current_pending = pending_component_removal;
    std::sort(current_pending.begin(), current_pending.end(), [](const pending_component_remove& left, const pending_component_remove& right) {
        if (left.owner == nullptr || left.component == nullptr)
            return false;
        if (right.owner == nullptr || right.component == nullptr)
            return true;
        if (left.owner->ID != right.owner->ID)
            return left.owner->ID < right.owner->ID;
        return left.component->key < right.component->key;
    });

    for (const pending_component_remove& pending_component : current_pending) {
        if (pending_component.owner == nullptr || pending_component.component == nullptr)
            continue;
        call_ondestroy(*pending_component.owner, *pending_component.component);
    }
}

void scene::process_event_subscriptions_changes() {
    if (pending_event_subscription_changes.empty())
        return;

    std::vector<pending_event_subscription_change> current_changes;
    current_changes.swap(pending_event_subscription_changes);

    for (const pending_event_subscription_change& change : current_changes) {
        actor* owner = nullptr;
        component_instance* component = find_component_instance(owner, change.component);
        if (owner == nullptr || component == nullptr || owner->pending_destroy || component->removed)
            continue;

        std::vector<event_subscription>& subscriptions = event_subscriptions[change.event_type];
        if (change.subscribe) {
            subscriptions.emplace_back(change.component, change.function);
            continue;
        }

        subscriptions.erase(
            std::remove_if(
                subscriptions.begin(),
                subscriptions.end(),
                [&](const event_subscription& subscription) {
                    return lua_refs_equal(lua_state, subscription.component, change.component) &&
                           lua_refs_equal(lua_state, subscription.function, change.function);
                }
            ),
            subscriptions.end()
        );

        if (subscriptions.empty())
            event_subscriptions.erase(change.event_type);
    }
}

void scene::handle_contact(actor* owner, actor* other, const b2Vec2& point, const b2Vec2& relative_velocity, const b2Vec2& normal, bool is_trigger, bool is_enter) {
    if (owner == nullptr || other == nullptr)
        return;

    for (const std::unique_ptr<component_instance>& component_ptr : owner->component_instances) {
        component_instance& component = *component_ptr;
        if (is_trigger && is_enter)
            call_component_contact_lifecycle(*owner, component, component.on_trigger_enter, other, point, relative_velocity, normal);
        else if (is_trigger)
            call_component_contact_lifecycle(*owner, component, component.on_trigger_exit, other, point, relative_velocity, normal);
        else if (is_enter)
            call_component_contact_lifecycle(*owner, component, component.on_collision_enter, other, point, relative_velocity, normal);
        else
            call_component_contact_lifecycle(*owner, component, component.on_collision_exit, other, point, relative_velocity, normal);
    }
}

void scene::handle_contact3d(actor* owner, actor* other, const glm::vec3& point, const glm::vec3& relative_velocity, const glm::vec3& normal, bool is_enter) {
    if (owner == nullptr || other == nullptr)
        return;

    for (const std::unique_ptr<component_instance>& component_ptr : owner->component_instances) {
        component_instance& component = *component_ptr;
        if (is_enter)
            call_component_contact_lifecycle3d(*owner, component, component.on_collision_enter, other, point, relative_velocity, normal);
        else
            call_component_contact_lifecycle3d(*owner, component, component.on_collision_exit, other, point, relative_velocity, normal);
    }
}

void scene::cleanup_end_of_frame() {
    pending_onstart.erase(
        std::remove_if(
            pending_onstart.begin(),
            pending_onstart.end(),
            [](const pending_component_start& pending_component) {
                return pending_component.owner == nullptr ||
                       pending_component.component == nullptr ||
                       pending_component.owner->pending_destroy ||
                       pending_component.component->removed;
            }
        ),
        pending_onstart.end()
    );

    for (const pending_component_remove& pending_component : pending_component_removal) {
        if (pending_component.owner == nullptr || pending_component.component == nullptr)
            continue;
        actor& owner = *pending_component.owner;
        remove_event_subscriptions_for_component(*this, pending_component.component->self);
        auto it = std::find_if(
            owner.component_instances.begin(),
            owner.component_instances.end(),
            [&](const std::unique_ptr<component_instance>& component_ptr) {
                return component_ptr.get() == pending_component.component;
            }
        );
        if (it != owner.component_instances.end() && (*it)->removed)
            owner.component_instances.erase(it);
    }
    pending_component_removal.clear();

    for (actor* actor_ptr : pending_actor_destroy) {
        if (actor_ptr == nullptr)
            continue;
        auto it = std::find_if(
            actors.begin(),
            actors.end(),
            [&](const std::unique_ptr<actor>& owned_actor) {
                return owned_actor.get() == actor_ptr;
            }
        );
        if (it != actors.end() && (*it)->pending_destroy)
            actors.erase(it);
    }
    pending_actor_destroy.clear();
}
