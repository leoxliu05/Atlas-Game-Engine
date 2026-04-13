#include "transform3d.h"

glm::mat4 Transform3D::GetTranslationMatrix() const {
    return glm::translate(glm::mat4(1.0f), position);
}

glm::mat4 Transform3D::GetRotationMatrix() const {
    glm::mat4 rotation = glm::mat4(1.0f);
    rotation = glm::rotate(rotation, glm::radians(rotation_degrees.x), glm::vec3(1.0f, 0.0f, 0.0f));
    rotation = glm::rotate(rotation, glm::radians(rotation_degrees.y), glm::vec3(0.0f, 1.0f, 0.0f));
    rotation = glm::rotate(rotation, glm::radians(rotation_degrees.z), glm::vec3(0.0f, 0.0f, 1.0f));
    return rotation;
}

glm::mat4 Transform3D::GetScaleMatrix() const {
    return glm::scale(glm::mat4(1.0f), scale);
}

glm::mat4 Transform3D::GetModelMatrix() const {
    return GetTranslationMatrix() * GetRotationMatrix() * GetScaleMatrix();
}

glm::vec3 Transform3D::GetRightVector() const {
    const glm::vec4 right = GetRotationMatrix() * glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
    return glm::normalize(glm::vec3(right));
}

glm::vec3 Transform3D::GetUpVector() const {
    const glm::vec4 up = GetRotationMatrix() * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
    return glm::normalize(glm::vec3(up));
}

glm::vec3 Transform3D::GetForwardVector() const {
    const glm::vec4 forward = GetRotationMatrix() * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f);
    return glm::normalize(glm::vec3(forward));
}
