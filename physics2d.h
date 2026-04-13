#ifndef PHYSICS2D_H
#define PHYSICS2D_H

#include "lua.hpp"
#include "LuaBridge/LuaBridge.h"
#include "box2d/box2d.h"
#include "glm.hpp"
#include <cstdint>
#include <memory>
#include <string>

struct actor;
class scene;

struct collision {
    actor* other = nullptr;
    b2Vec2 point = b2Vec2(-999.0f, -999.0f);
    b2Vec2 relative_velocity = b2Vec2_zero;
    b2Vec2 normal = b2Vec2(-999.0f, -999.0f);
};

class physics2d {
public:
    inline static std::unique_ptr<b2World> world = nullptr;

    static b2World* GetWorld();
    static void EnsureWorld(scene& current_scene);
    static void StepWorld(scene& current_scene);
    static void Shutdown();
    static luabridge::LuaRef Raycast(const b2Vec2& pos, const b2Vec2& dir, float dist); // api 2d
    static luabridge::LuaRef RaycastAll(const b2Vec2& pos, const b2Vec2& dir, float dist); // api 2d
};

constexpr uint16 collider_category_bits = 0x0001;
constexpr uint16 trigger_category_bits = 0x0002;
constexpr uint16 phantom_category_bits = 0x0004;

class Rigidbody {
public:
    actor* owner = nullptr;
    std::string key = "";
    bool enabled = true;

    float x = 0.0f;
    float y = 0.0f;
    std::string body_type = "dynamic";
    bool precise = true;
    float gravity_scale = 1.0f;
    float density = 1.0f;
    float angular_friction = 0.3f;
    float rotation = 0.0f;
    bool has_collider = true;
    bool has_trigger = true;
    std::string collider_type = "box";
    float width = 1.0f;
    float height = 1.0f;
    float radius = 0.5f;
    float friction = 0.3f;
    float bounciness = 0.3f;
    std::string trigger_type = "box";
    float trigger_width = 1.0f;
    float trigger_height = 1.0f;
    float trigger_radius = 0.5f;
    b2Vec2 linear_velocity = b2Vec2_zero;
    float angular_velocity = 0.0f;

    b2Body* body = nullptr;

    ~Rigidbody() {
        OnDestroy();
    }

    b2Vec2 GetPosition() const { // api 2d
        if (body == nullptr)
            return b2Vec2(x, y);
        return body->GetPosition();
    }
    float GetRotation() const { // api 2d
        if (body == nullptr)
            return rotation;
        return body->GetAngle() * (180.0f / b2_pi);
    }
    void AddForce(const b2Vec2& force) { // api 2d
        if (body == nullptr)
            return;
        body->ApplyForceToCenter(force, true);
    }
    void SetPosition(const b2Vec2& position) { // api 2d
        if (body == nullptr) {
            x = position.x;
            y = position.y;
            return;
        }
        body->SetTransform(position, body->GetAngle());
    }
    void SetRotation(float degrees_clockwise) { // api 2d
        if (body == nullptr) {
            rotation = degrees_clockwise;
            return;
        }
        body->SetTransform(body->GetPosition(), degrees_clockwise * (b2_pi / 180.0f));
    }
    b2Vec2 GetVelocity() const { // api 2d
        if (body == nullptr)
            return linear_velocity;
        return body->GetLinearVelocity();
    }
    void SetVelocity(const b2Vec2& velocity) { // api 2d
        if (body == nullptr) {
            linear_velocity = velocity;
            return;
        }
        body->SetLinearVelocity(velocity);
    }
    float GetAngularVelocity() const { // api 2d
        if (body == nullptr)
            return angular_velocity;
        return body->GetAngularVelocity() * (180.0f / b2_pi);
    }
    void SetAngularVelocity(float degrees_clockwise_per_second) { // api 2d
        if (body == nullptr) {
            angular_velocity = degrees_clockwise_per_second;
            return;
        }
        body->SetAngularVelocity(degrees_clockwise_per_second * (b2_pi / 180.0f));
    }
    float GetGravityScale() const { // api 2d
        if (body == nullptr)
            return gravity_scale;
        return body->GetGravityScale();
    }
    void SetGravityScale(float new_gravity_scale) { // api 2d
        if (body == nullptr) {
            gravity_scale = new_gravity_scale;
            return;
        }
        body->SetGravityScale(new_gravity_scale);
    }
    b2Vec2 GetRightDirection() const { // api 2d
        float angle = body == nullptr ? rotation * (b2_pi / 180.0f) : body->GetAngle();
        b2Vec2 result = b2Vec2(glm::cos(angle), glm::sin(angle));
        result.Normalize();
        return result;
    }
    b2Vec2 GetUpDirection() const { // api 2d
        float angle = body == nullptr ? rotation * (b2_pi / 180.0f) : body->GetAngle();
        b2Vec2 result = b2Vec2(glm::sin(angle), -glm::cos(angle));
        result.Normalize();
        return result;
    }
    void SetRightDirection(const b2Vec2& direction) { // api 2d
        b2Vec2 normalized = direction;
        if (normalized.Normalize() == 0.0f)
            return;
        SetRotation(glm::atan(normalized.y, normalized.x) * (180.0f / b2_pi));
    }
    void SetUpDirection(const b2Vec2& direction) { // api 2d
        b2Vec2 normalized = direction;
        if (normalized.Normalize() == 0.0f)
            return;
        SetRotation(glm::atan(normalized.x, -normalized.y) * (180.0f / b2_pi));
    }
    void OnStart() {
        if (body != nullptr)
            return;

        b2World* current_world = physics2d::GetWorld();
        if (current_world == nullptr)
            return;

        b2BodyDef body_def;
        if (body_type == "static")
            body_def.type = b2_staticBody;
        else if (body_type == "kinematic")
            body_def.type = b2_kinematicBody;
        else
            body_def.type = b2_dynamicBody;

        body_def.position.Set(x, y);
        body_def.angle = rotation * (b2_pi / 180.0f);
        body_def.linearVelocity = linear_velocity;
        body_def.angularVelocity = angular_velocity * (b2_pi / 180.0f);
        body_def.bullet = precise;
        body_def.gravityScale = gravity_scale;
        body_def.angularDamping = angular_friction;

        body = current_world->CreateBody(&body_def);

        if (!has_collider && !has_trigger) {
            b2PolygonShape phantom_shape;
            phantom_shape.SetAsBox(width * 0.5f, height * 0.5f);

            b2FixtureDef phantom_fixture_def;
            phantom_fixture_def.shape = &phantom_shape;
            phantom_fixture_def.density = density;
            phantom_fixture_def.isSensor = true;
            phantom_fixture_def.filter.categoryBits = phantom_category_bits;
            phantom_fixture_def.filter.maskBits = 0x0000;
            b2Fixture* fixture = body->CreateFixture(&phantom_fixture_def);
            fixture->GetUserData().pointer = 0;
        }

        if (has_collider) {
            b2FixtureDef fixture_def;
            fixture_def.isSensor = false;
            fixture_def.density = density;
            fixture_def.friction = friction;
            fixture_def.restitution = bounciness;
            fixture_def.filter.categoryBits = collider_category_bits;
            fixture_def.filter.maskBits = collider_category_bits;

            if (collider_type == "circle") {
                b2CircleShape shape;
                shape.m_radius = radius;
                fixture_def.shape = &shape;
                b2Fixture* fixture = body->CreateFixture(&fixture_def);
                fixture->GetUserData().pointer = reinterpret_cast<uintptr_t>(owner);
            } else if (collider_type == "box") {
                b2PolygonShape shape;
                shape.SetAsBox(width * 0.5f, height * 0.5f);
                fixture_def.shape = &shape;
                b2Fixture* fixture = body->CreateFixture(&fixture_def);
                fixture->GetUserData().pointer = reinterpret_cast<uintptr_t>(owner);
            }
        }

        if (has_trigger) {
            b2FixtureDef fixture_def;
            fixture_def.density = density;
            fixture_def.isSensor = true;
            fixture_def.filter.categoryBits = trigger_category_bits;
            fixture_def.filter.maskBits = trigger_category_bits;

            if (trigger_type == "circle") {
                b2CircleShape shape;
                shape.m_radius = trigger_radius;
                fixture_def.shape = &shape;
                b2Fixture* fixture = body->CreateFixture(&fixture_def);
                fixture->GetUserData().pointer = reinterpret_cast<uintptr_t>(owner);
            } else if (trigger_type == "box") {
                b2PolygonShape shape;
                shape.SetAsBox(trigger_width * 0.5f, trigger_height * 0.5f);
                fixture_def.shape = &shape;
                b2Fixture* fixture = body->CreateFixture(&fixture_def);
                fixture->GetUserData().pointer = reinterpret_cast<uintptr_t>(owner);
            }
        }
    }

    void OnDestroy() {
        b2World* current_world = physics2d::GetWorld();
        if (current_world == nullptr || body == nullptr)
            return;
        current_world->DestroyBody(body);
        body = nullptr;
    }
};

#endif
