#include "physics2d.h"
#include "engine.h"
#include "scene_DB.h"
#include <algorithm>
#include <vector>

namespace {

struct raycast_hit {
    actor* actor = nullptr;
    b2Vec2 point = b2Vec2_zero;
    b2Vec2 normal = b2Vec2_zero;
    bool is_trigger = false;
    float distance_fraction = 1.0f;
};

class physics_raycast_callback : public b2RayCastCallback {
public:
    std::vector<raycast_hit> hits;

    float ReportFixture(b2Fixture* fixture,
                        const b2Vec2& point,
                        const b2Vec2& normal,
                        float fraction) override {
        if (fixture == nullptr)
            return -1.0f;

        actor* hit_actor = reinterpret_cast<actor*>(fixture->GetUserData().pointer);
        if (hit_actor == nullptr)
            return -1.0f;

        hits.emplace_back(raycast_hit{
            hit_actor,
            point,
            normal,
            fixture->IsSensor(),
            fraction
        });
        return 1.0f;
    }
};

luabridge::LuaRef make_hit_result_lua(lua_State* lua_state, const raycast_hit& hit) {
    luabridge::LuaRef result = luabridge::newTable(lua_state);
    result["actor"] = hit.actor;
    result["point"] = hit.point;
    result["normal"] = hit.normal;
    result["is_trigger"] = hit.is_trigger;
    return result;
}

std::vector<raycast_hit> collect_raycast_hits(const b2Vec2& pos, const b2Vec2& dir, float dist) {
    std::vector<raycast_hit> hits;
    b2World* current_world = physics2d::GetWorld();
    if (current_world == nullptr || dist <= 0.0f)
        return hits;

    b2Vec2 direction = dir;
    if (direction.Normalize() == 0.0f)
        return hits;

    physics_raycast_callback callback;
    current_world->RayCast(&callback, pos, pos + (dist * direction));
    hits = std::move(callback.hits);
    std::sort(hits.begin(), hits.end(), [](const raycast_hit& left, const raycast_hit& right) {
        return left.distance_fraction < right.distance_fraction;
    });
    return hits;
}

class physics_contact_listener : public b2ContactListener {
public:
    void BeginContact(b2Contact* contact) override {
        handle_contact_callbacks(contact, true);
    }

    void EndContact(b2Contact* contact) override {
        handle_contact_callbacks(contact, false);
    }

private:
    static void handle_contact_callbacks(b2Contact* contact, bool is_enter) {
        if (contact == nullptr || scene::current_scene == nullptr)
            return;

        b2Fixture* fixture_a = contact->GetFixtureA();
        b2Fixture* fixture_b = contact->GetFixtureB();
        if (fixture_a == nullptr || fixture_b == nullptr)
            return;

        const bool fixture_a_is_sensor = fixture_a->IsSensor();
        const bool fixture_b_is_sensor = fixture_b->IsSensor();
        if (fixture_a_is_sensor != fixture_b_is_sensor)
            return;

        const bool is_trigger = fixture_a_is_sensor;
        actor* actor_a = reinterpret_cast<actor*>(fixture_a->GetUserData().pointer);
        actor* actor_b = reinterpret_cast<actor*>(fixture_b->GetUserData().pointer);
        if (actor_a == nullptr || actor_b == nullptr)
            return;

        b2WorldManifold world_manifold;
        contact->GetWorldManifold(&world_manifold);
        const b2Vec2 contact_point = world_manifold.points[0];
        const b2Vec2 contact_normal = world_manifold.normal;

        b2Body* body_a = fixture_a->GetBody();
        b2Body* body_b = fixture_b->GetBody();
        const b2Vec2 relative_velocity = body_a->GetLinearVelocity() - body_b->GetLinearVelocity();

        const b2Vec2 invalid = b2Vec2(-999.0f, -999.0f);
        const b2Vec2 point = (is_enter && !is_trigger) ? contact_point : invalid;
        const b2Vec2 normal = (is_enter && !is_trigger) ? contact_normal : invalid;

        scene::handle_contact(actor_a, actor_b, point, relative_velocity, normal, is_trigger, is_enter);
        scene::handle_contact(actor_b, actor_a, point, relative_velocity, normal, is_trigger, is_enter);
    }
};

physics_contact_listener world_contact_listener;

}

b2World* physics2d::GetWorld() {
    return world.get();
}

void physics2d::EnsureWorld(scene& current_scene) {
    if (world != nullptr)
        return;
    if (current_scene.loaded_component_types.find("Rigidbody") == current_scene.loaded_component_types.end())
        return;

    world = std::make_unique<b2World>(b2Vec2(0.0f, 9.8f));
    world->SetContactListener(&world_contact_listener);
}

void physics2d::StepWorld(scene& current_scene) {
    EnsureWorld(current_scene);
    if (world != nullptr)
        world->Step(1.0f / 60.0f, 8, 3);
}

void physics2d::Shutdown() {
    if (world != nullptr)
        world->SetContactListener(nullptr);
    world.reset();
}

luabridge::LuaRef physics2d::Raycast(const b2Vec2& pos, const b2Vec2& dir, float dist) {
    lua_State* lua_state = engine::current_engine->lua_state;
    std::vector<raycast_hit> hits = collect_raycast_hits(pos, dir, dist);
    if (hits.empty())
        return luabridge::LuaRef(lua_state);
    return make_hit_result_lua(lua_state, hits.front());
}

luabridge::LuaRef physics2d::RaycastAll(const b2Vec2& pos, const b2Vec2& dir, float dist) {
    lua_State* lua_state = engine::current_engine->lua_state;
    std::vector<raycast_hit> hits = collect_raycast_hits(pos, dir, dist);
    luabridge::LuaRef results = luabridge::newTable(lua_state);
    int index = 1;
    for (const raycast_hit& hit : hits) {
        results[index] = make_hit_result_lua(lua_state, hit);
        index++;
    }
    return results;
}
