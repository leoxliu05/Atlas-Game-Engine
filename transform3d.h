#ifndef TRANSFORM3D_H
#define TRANSFORM3D_H

#include "glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

struct Transform3D {
    glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 rotation_degrees = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 scale = glm::vec3(1.0f, 1.0f, 1.0f);

    glm::mat4 GetTranslationMatrix() const;
    glm::mat4 GetRotationMatrix() const;
    glm::mat4 GetScaleMatrix() const;
    glm::mat4 GetModelMatrix() const;

    glm::vec3 GetRightVector() const; // api 3d
    glm::vec3 GetUpVector() const; // api 3d
    glm::vec3 GetForwardVector() const; // api 3d
};

#endif // TRANSFORM3D_H
