#ifndef DIRECTIONAL_LIGHT3D_H
#define DIRECTIONAL_LIGHT3D_H

#include "transform3d.h"
#include "glm.hpp"

struct DirectionalLight3D {
    Transform3D transform;
    glm::vec3 color = glm::vec3(1.0f, 0.96f, 0.84f);
    float intensity = 1.0f;

    glm::vec3 GetDirection() const // api 3d
    {
        return transform.GetForwardVector();
    }

    glm::vec3 GetRadiance() const // api 3d
    {
        return color * intensity;
    }

    void SetDirection(const glm::vec3& direction) // api 3d
    {
        if (glm::length(direction) <= 0.00001f)
            return;

        const glm::vec3 normalized = glm::normalize(direction);
        transform.rotation_degrees.x = glm::degrees(glm::asin(normalized.y));
        transform.rotation_degrees.y = glm::degrees(glm::atan(-normalized.x, -normalized.z));
        transform.rotation_degrees.z = 0.0f;
    }
};

#endif // DIRECTIONAL_LIGHT3D_H
