#ifndef MATH_UTILS_H
#define MATH_UTILS_H

#include "box2d/box2d.h"
#include "glm.hpp"

class Math {
public:
    static glm::vec2 Vec2Add(const glm::vec2& left, const glm::vec2& right)
    {
        return left + right;
    }

    static glm::vec2 Vec2OperatorAdd(const glm::vec2* left, const glm::vec2& right)
    {
        return *left + right;
    }

    static glm::vec2 Vec2Subtract(const glm::vec2& left, const glm::vec2& right)
    {
        return left - right;
    }

    static glm::vec2 Vec2OperatorSubtract(const glm::vec2* left, const glm::vec2& right)
    {
        return *left - right;
    }

    static glm::vec2 Vec2Multiply(const glm::vec2& left, float scalar)
    {
        return left * scalar;
    }

    static glm::vec2 Vec2OperatorMultiply(const glm::vec2* left, float scalar)
    {
        return *left * scalar;
    }

    static float Vec2Length(const glm::vec2& value)
    {
        return glm::length(value);
    }

    static glm::vec2 Vec2Normalize(const glm::vec2& value)
    {
        return glm::normalize(value);
    }

    static float Vec2Distance(const glm::vec2& left, const glm::vec2& right)
    {
        return glm::distance(left, right);
    }

    static float Vec2Dot(const glm::vec2& left, const glm::vec2& right)
    {
        return glm::dot(left, right);
    }

    static float Vector2Distance(const b2Vec2& left, const b2Vec2& right)
    {
        return b2Distance(left, right);
    }

    static float Vector2Dot(const b2Vec2& left, const b2Vec2& right)
    {
        return b2Dot(left, right);
    }

    static glm::vec3 Vector3Add(const glm::vec3& left, const glm::vec3& right)
    {
        return left + right;
    }

    static glm::vec3 Vector3OperatorAdd(const glm::vec3* left, const glm::vec3& right)
    {
        return *left + right;
    }

    static glm::vec3 Vector3Subtract(const glm::vec3& left, const glm::vec3& right)
    {
        return left - right;
    }

    static glm::vec3 Vector3OperatorSubtract(const glm::vec3* left, const glm::vec3& right)
    {
        return *left - right;
    }

    static glm::vec3 Vector3Multiply(const glm::vec3& left, float scalar)
    {
        return left * scalar;
    }

    static glm::vec3 Vector3OperatorMultiply(const glm::vec3* left, float scalar)
    {
        return *left * scalar;
    }

    static glm::vec3 Vector3MultiplyScalarLeft(float scalar, const glm::vec3& right)
    {
        return scalar * right;
    }

    static float Vector3Length(const glm::vec3& value)
    {
        return glm::length(value);
    }

    static glm::vec3 Vector3Normalize(const glm::vec3& value)
    {
        return glm::normalize(value);
    }

    static float Vector3Dot(const glm::vec3& left, const glm::vec3& right)
    {
        return glm::dot(left, right);
    }

    static glm::vec3 Vector3Cross(const glm::vec3& left, const glm::vec3& right)
    {
        return glm::cross(left, right);
    }

    static float Vector3Distance(const glm::vec3& left, const glm::vec3& right)
    {
        return glm::distance(left, right);
    }
};

#endif
