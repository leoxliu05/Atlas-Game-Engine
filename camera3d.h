#ifndef CAMERA3D_H
#define CAMERA3D_H

#include "glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "transform3d.h"

struct Camera3D {
    Transform3D transform;
    float field_of_view_degrees = 60.0f;
    float aspect_ratio = 16.0f / 9.0f;
    float near_clip = 0.1f;
    float far_clip = 1000.0f;
    bool is_orthographic = false;
    float orthographic_height = 10.0f;

    glm::mat4 GetViewMatrix() const;
    glm::mat4 GetProjectionMatrix() const;
    glm::mat4 GetViewProjectionMatrix() const;
};

#endif // CAMERA3D_H
