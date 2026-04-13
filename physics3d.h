#ifndef PHYSICS3D_H
#define PHYSICS3D_H

#include "lua.hpp"
#include "LuaBridge/LuaBridge.h"
#include "bullet/btBulletDynamicsCommon.h"
#include "glm.hpp"
#include <memory>
#include <string>

struct actor;
class scene;

struct collision3d {
    actor* other = nullptr;
    glm::vec3 point = glm::vec3(-999.0f, -999.0f, -999.0f);
    glm::vec3 relative_velocity = glm::vec3(0.0f);
    glm::vec3 normal = glm::vec3(-999.0f, -999.0f, -999.0f);
};

class physics3d {
public:
    inline static std::unique_ptr<btDefaultCollisionConfiguration> collision_configuration = nullptr;
    inline static std::unique_ptr<btCollisionDispatcher> dispatcher = nullptr;
    inline static std::unique_ptr<btBroadphaseInterface> broadphase = nullptr;
    inline static std::unique_ptr<btSequentialImpulseConstraintSolver> solver = nullptr;
    inline static std::unique_ptr<btDiscreteDynamicsWorld> world = nullptr;

    static btDiscreteDynamicsWorld* GetWorld();
    static void EnsureWorld(scene& current_scene);
    static void StepWorld(scene& current_scene);
    static bool OverlapBox(const glm::vec3& position, const glm::vec3& half_extents);
    static luabridge::LuaRef Raycast(const glm::vec3& origin, const glm::vec3& direction, float distance);
    static void Shutdown();
};

class Rigidbody3D {
public:
    actor* owner = nullptr;
    std::string key = "";
    bool enabled = true;

    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotation_degrees = glm::vec3(0.0f);
    std::string body_type = "dynamic";
    std::string collider_type = "box";
    bool precise = false;
    float mass = 1.0f;
    float gravity_scale = 1.0f;
    float linear_damping = 0.0f;
    float angular_damping = 0.2f;
    float width = 1.0f;
    float height = 1.0f;
    float depth = 1.0f;
    float radius = 0.5f;
    float friction = 0.5f;
    float bounciness = 0.0f;
    glm::vec3 linear_velocity = glm::vec3(0.0f);
    glm::vec3 angular_velocity_degrees = glm::vec3(0.0f);

    btCollisionShape* shape = nullptr;
    btDefaultMotionState* motion_state = nullptr;
    btRigidBody* body = nullptr;

    ~Rigidbody3D();

    glm::vec3 GetPosition() const;
    glm::vec3 GetRotation() const;
    void SetPosition(const glm::vec3& new_position);
    void SetRotation(const glm::vec3& new_rotation_degrees);
    glm::vec3 GetVelocity() const;
    void SetVelocity(const glm::vec3& new_velocity);
    glm::vec3 GetAngularVelocity() const;
    void SetAngularVelocity(const glm::vec3& new_angular_velocity_degrees);
    float GetGravityScale() const;
    void SetGravityScale(float new_gravity_scale);
    void AddForce(const glm::vec3& force);
    void OnStart();
    void OnDestroy();
};

#endif
