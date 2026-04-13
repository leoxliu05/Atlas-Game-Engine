#include "physics3d.h"
#include "engine.h"
#include "scene_DB.h"
#include "bullet/btBulletDynamicsCommon.h"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/quaternion.hpp"
#include <unordered_map>

namespace {

constexpr float radians_per_degree = 0.01745329251994329577f;
constexpr float degrees_per_radian = 57.295779513082320876f;
const btVector3 world_gravity(0.0f, -9.8f, 0.0f);

btVector3 to_bt(const glm::vec3& value) {
    return btVector3(value.x, value.y, value.z);
}

glm::vec3 to_glm(const btVector3& value) {
    return glm::vec3(value.x(), value.y(), value.z());
}

btQuaternion to_bt_rotation(const glm::vec3& rotation_degrees) {
    glm::mat4 rotation = glm::mat4(1.0f);
    rotation = glm::rotate(rotation, glm::radians(rotation_degrees.x), glm::vec3(1.0f, 0.0f, 0.0f));
    rotation = glm::rotate(rotation, glm::radians(rotation_degrees.y), glm::vec3(0.0f, 1.0f, 0.0f));
    rotation = glm::rotate(rotation, glm::radians(rotation_degrees.z), glm::vec3(0.0f, 0.0f, 1.0f));
    const glm::quat quat = glm::quat_cast(rotation);
    return btQuaternion(quat.x, quat.y, quat.z, quat.w);
}

glm::vec3 to_glm_rotation_degrees(const btQuaternion& rotation) {
    const glm::quat quat(rotation.w(), rotation.x(), rotation.y(), rotation.z());
    return glm::degrees(glm::eulerAngles(quat));
}

btTransform make_transform(const glm::vec3& position, const glm::vec3& rotation_degrees) {
    btTransform transform;
    transform.setIdentity();
    transform.setOrigin(to_bt(position));
    transform.setRotation(to_bt_rotation(rotation_degrees));
    return transform;
}

float compute_body_mass(const Rigidbody3D& rigidbody) {
    if (rigidbody.body_type == "static" || rigidbody.body_type == "kinematic")
        return 0.0f;
    return rigidbody.mass < 0.0f ? 0.0f : rigidbody.mass;
}

btDiscreteDynamicsWorld* ensure_world_from_engine() {
    if (engine::current_engine == nullptr)
        return nullptr;
    physics3d::EnsureWorld(engine::current_engine->my_scene);
    return physics3d::GetWorld();
}

class overlap_callback final : public btCollisionWorld::ContactResultCallback {
public:
    bool hit = false;

    btScalar addSingleResult(btManifoldPoint&,
                             const btCollisionObjectWrapper*,
                             int,
                             int,
                             const btCollisionObjectWrapper*,
                             int,
                             int) override {
        hit = true;
        return 0.0f;
    }
};

struct contact_pair_3d {
    actor* actor_a = nullptr;
    actor* actor_b = nullptr;
    glm::vec3 point = glm::vec3(-999.0f, -999.0f, -999.0f);
    glm::vec3 relative_velocity = glm::vec3(0.0f);
    glm::vec3 normal = glm::vec3(-999.0f, -999.0f, -999.0f);
};

std::unordered_map<std::string, contact_pair_3d> previous_contacts_3d;

std::string make_contact_key(const btCollisionObject* object_a, const btCollisionObject* object_b) {
    const auto address_a = reinterpret_cast<std::uintptr_t>(object_a);
    const auto address_b = reinterpret_cast<std::uintptr_t>(object_b);
    if (address_a < address_b)
        return std::to_string(address_a) + ":" + std::to_string(address_b);
    return std::to_string(address_b) + ":" + std::to_string(address_a);
}

glm::vec3 get_body_velocity(const btCollisionObject* object) {
    const btRigidBody* body = btRigidBody::upcast(object);
    if (body == nullptr)
        return glm::vec3(0.0f);
    return to_glm(body->getLinearVelocity());
}

void dispatch_contact_events(scene& current_scene) {
    if (physics3d::world == nullptr || physics3d::dispatcher == nullptr) {
        previous_contacts_3d.clear();
        return;
    }

    std::unordered_map<std::string, contact_pair_3d> current_contacts;
    const int manifold_count = physics3d::dispatcher->getNumManifolds();
    for (int manifold_index = 0; manifold_index < manifold_count; ++manifold_index) {
        btPersistentManifold* manifold = physics3d::dispatcher->getManifoldByIndexInternal(manifold_index);
        if (manifold == nullptr || manifold->getNumContacts() <= 0)
            continue;

        const btCollisionObject* object_a = manifold->getBody0();
        const btCollisionObject* object_b = manifold->getBody1();
        if (object_a == nullptr || object_b == nullptr)
            continue;

        actor* actor_a = reinterpret_cast<actor*>(object_a->getUserPointer());
        actor* actor_b = reinterpret_cast<actor*>(object_b->getUserPointer());
        if (actor_a == nullptr || actor_b == nullptr)
            continue;

        const btManifoldPoint& point = manifold->getContactPoint(0);
        if (point.getDistance() > 0.0f)
            continue;

        contact_pair_3d contact;
        contact.actor_a = actor_a;
        contact.actor_b = actor_b;
        contact.point = to_glm(point.getPositionWorldOnB());
        contact.normal = to_glm(point.m_normalWorldOnB);
        contact.relative_velocity = get_body_velocity(object_a) - get_body_velocity(object_b);

        current_contacts[make_contact_key(object_a, object_b)] = contact;
    }

    for (const auto& [key, contact] : current_contacts) {
        if (previous_contacts_3d.find(key) != previous_contacts_3d.end())
            continue;
        scene::handle_contact3d(contact.actor_a, contact.actor_b, contact.point, contact.relative_velocity, contact.normal, true);
        scene::handle_contact3d(contact.actor_b, contact.actor_a, contact.point, contact.relative_velocity, contact.normal, true);
    }

    const glm::vec3 invalid_point(-999.0f, -999.0f, -999.0f);
    for (const auto& [key, contact] : previous_contacts_3d) {
        if (current_contacts.find(key) != current_contacts.end())
            continue;
        scene::handle_contact3d(contact.actor_a, contact.actor_b, invalid_point, contact.relative_velocity, invalid_point, false);
        scene::handle_contact3d(contact.actor_b, contact.actor_a, invalid_point, contact.relative_velocity, invalid_point, false);
    }

    previous_contacts_3d = std::move(current_contacts);
}

luabridge::LuaRef make_raycast_hit_lua(lua_State* lua_state, const btCollisionWorld::ClosestRayResultCallback& callback) {
    luabridge::LuaRef result = luabridge::newTable(lua_state);
    result["actor"] = reinterpret_cast<actor*>(const_cast<void*>(callback.m_collisionObject->getUserPointer()));
    result["point"] = to_glm(callback.m_hitPointWorld);
    result["normal"] = to_glm(callback.m_hitNormalWorld);
    result["distance_fraction"] = callback.m_closestHitFraction;
    return result;
}

}

btDiscreteDynamicsWorld* physics3d::GetWorld() {
    return world.get();
}

void physics3d::EnsureWorld(scene& current_scene) {
    if (world != nullptr)
        return;
    if (current_scene.loaded_component_types.find("Rigidbody3D") == current_scene.loaded_component_types.end())
        return;

    collision_configuration = std::make_unique<btDefaultCollisionConfiguration>();
    dispatcher = std::make_unique<btCollisionDispatcher>(collision_configuration.get());
    broadphase = std::make_unique<btDbvtBroadphase>();
    solver = std::make_unique<btSequentialImpulseConstraintSolver>();
    world = std::make_unique<btDiscreteDynamicsWorld>(dispatcher.get(), broadphase.get(), solver.get(), collision_configuration.get());
    world->setGravity(world_gravity);
}

void physics3d::StepWorld(scene& current_scene) {
    EnsureWorld(current_scene);
    if (world != nullptr) {
        world->stepSimulation(1.0f / 60.0f, 2, 1.0f / 60.0f);
        dispatch_contact_events(current_scene);
    }
}

bool physics3d::OverlapBox(const glm::vec3& position, const glm::vec3& half_extents) {
    btDiscreteDynamicsWorld* current_world = ensure_world_from_engine();
    if (current_world == nullptr)
        return false;

    btBoxShape query_shape(to_bt(half_extents));
    query_shape.setMargin(0.0f);
    btCollisionObject query_object;
    query_object.setCollisionShape(&query_shape);
    query_object.setWorldTransform(make_transform(position, glm::vec3(0.0f)));

    overlap_callback callback;
    current_world->contactTest(&query_object, callback);
    return callback.hit;
}

luabridge::LuaRef physics3d::Raycast(const glm::vec3& origin, const glm::vec3& direction, float distance) {
    lua_State* lua_state = engine::current_engine->lua_state;
    btDiscreteDynamicsWorld* current_world = ensure_world_from_engine();
    if (current_world == nullptr || distance <= 0.0f)
        return luabridge::LuaRef(lua_state);

    glm::vec3 normalized_direction = direction;
    if (glm::length(normalized_direction) <= 0.0001f)
        return luabridge::LuaRef(lua_state);
    normalized_direction = glm::normalize(normalized_direction);

    const btVector3 from = to_bt(origin);
    const btVector3 to = from + to_bt(normalized_direction * distance);
    btCollisionWorld::ClosestRayResultCallback callback(from, to);
    current_world->rayTest(from, to, callback);
    if (!callback.hasHit())
        return luabridge::LuaRef(lua_state);

    return make_raycast_hit_lua(lua_state, callback);
}

void physics3d::Shutdown() {
    previous_contacts_3d.clear();
    world.reset();
    solver.reset();
    broadphase.reset();
    dispatcher.reset();
    collision_configuration.reset();
}

Rigidbody3D::~Rigidbody3D() {
    OnDestroy();
}

glm::vec3 Rigidbody3D::GetPosition() const {
    if (body == nullptr)
        return position;
    btTransform transform;
    if (motion_state != nullptr)
        motion_state->getWorldTransform(transform);
    else
        transform = body->getWorldTransform();
    return to_glm(transform.getOrigin());
}

glm::vec3 Rigidbody3D::GetRotation() const {
    if (body == nullptr)
        return rotation_degrees;
    btTransform transform;
    if (motion_state != nullptr)
        motion_state->getWorldTransform(transform);
    else
        transform = body->getWorldTransform();
    return to_glm_rotation_degrees(transform.getRotation());
}

void Rigidbody3D::SetPosition(const glm::vec3& new_position) {
    position = new_position;
    if (body == nullptr)
        return;
    btTransform transform = body->getWorldTransform();
    transform.setOrigin(to_bt(new_position));
    body->setWorldTransform(transform);
    if (motion_state != nullptr)
        motion_state->setWorldTransform(transform);
    body->activate(true);
}

void Rigidbody3D::SetRotation(const glm::vec3& new_rotation_degrees) {
    rotation_degrees = new_rotation_degrees;
    if (body == nullptr)
        return;
    btTransform transform = body->getWorldTransform();
    transform.setRotation(to_bt_rotation(new_rotation_degrees));
    body->setWorldTransform(transform);
    if (motion_state != nullptr)
        motion_state->setWorldTransform(transform);
    body->activate(true);
}

glm::vec3 Rigidbody3D::GetVelocity() const {
    if (body == nullptr)
        return linear_velocity;
    return to_glm(body->getLinearVelocity());
}

void Rigidbody3D::SetVelocity(const glm::vec3& new_velocity) {
    linear_velocity = new_velocity;
    if (body == nullptr)
        return;
    body->setLinearVelocity(to_bt(new_velocity));
    body->activate(true);
}

glm::vec3 Rigidbody3D::GetAngularVelocity() const {
    if (body == nullptr)
        return angular_velocity_degrees;
    return to_glm(body->getAngularVelocity()) * degrees_per_radian;
}

void Rigidbody3D::SetAngularVelocity(const glm::vec3& new_angular_velocity_degrees) {
    angular_velocity_degrees = new_angular_velocity_degrees;
    if (body == nullptr)
        return;
    body->setAngularVelocity(to_bt(new_angular_velocity_degrees * radians_per_degree));
    body->activate(true);
}

float Rigidbody3D::GetGravityScale() const {
    return gravity_scale;
}

void Rigidbody3D::SetGravityScale(float new_gravity_scale) {
    gravity_scale = new_gravity_scale;
    if (body == nullptr)
        return;
    body->setGravity(world_gravity * gravity_scale);
}

void Rigidbody3D::AddForce(const glm::vec3& force) {
    if (body == nullptr)
        return;
    body->applyCentralForce(to_bt(force));
    body->activate(true);
}

void Rigidbody3D::OnStart() {
    if (body != nullptr)
        return;

    btDiscreteDynamicsWorld* current_world = physics3d::GetWorld();
    if (current_world == nullptr)
        return;

    if (collider_type == "sphere")
        shape = new btSphereShape(radius);
    else
        shape = new btBoxShape(btVector3(width * 0.5f, height * 0.5f, depth * 0.5f));

    const float body_mass = compute_body_mass(*this);
    btVector3 local_inertia(0.0f, 0.0f, 0.0f);
    if (body_mass > 0.0f)
        shape->calculateLocalInertia(body_mass, local_inertia);

    motion_state = new btDefaultMotionState(make_transform(position, rotation_degrees));
    btRigidBody::btRigidBodyConstructionInfo body_info(body_mass, motion_state, shape, local_inertia);
    body_info.m_friction = friction;
    body_info.m_restitution = bounciness;
    body_info.m_linearDamping = linear_damping;
    body_info.m_angularDamping = angular_damping;

    body = new btRigidBody(body_info);
    body->setUserPointer(owner);
    body->setGravity(world_gravity * gravity_scale);
    body->setLinearVelocity(to_bt(linear_velocity));
    body->setAngularVelocity(to_bt(angular_velocity_degrees * radians_per_degree));
    if (precise)
        body->setCcdMotionThreshold(0.01f);

    if (body_type == "kinematic") {
        body->setCollisionFlags(body->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
        body->setActivationState(DISABLE_DEACTIVATION);
    }

    current_world->addRigidBody(body);
}

void Rigidbody3D::OnDestroy() {
    btDiscreteDynamicsWorld* current_world = physics3d::GetWorld();
    if (current_world != nullptr && body != nullptr)
        current_world->removeRigidBody(body);

    delete body;
    body = nullptr;

    delete motion_state;
    motion_state = nullptr;

    delete shape;
    shape = nullptr;
}
