#include "camera3d.h"

glm::mat4 Camera3D::GetViewMatrix() const {
    const float yaw_radians = glm::radians(transform.rotation_degrees.y);
    const float pitch_radians = glm::radians(transform.rotation_degrees.x);

    glm::vec3 forward;
    forward.x = -glm::sin(yaw_radians) * glm::cos(pitch_radians);
    forward.y = glm::sin(pitch_radians);
    forward.z = -glm::cos(yaw_radians) * glm::cos(pitch_radians);
    forward = glm::normalize(forward);

    const glm::vec3 world_up(0.0f, 1.0f, 0.0f);
    const glm::vec3 right = glm::normalize(glm::cross(forward, world_up));
    const glm::vec3 up = glm::normalize(glm::cross(right, forward));

    return glm::lookAt(transform.position, transform.position + forward, up);
}

glm::mat4 Camera3D::GetProjectionMatrix() const {
    if (is_orthographic) {
        const float half_height = orthographic_height * 0.5f;
        const float half_width = half_height * aspect_ratio;
        return glm::ortho(-half_width, half_width, -half_height, half_height, near_clip, far_clip);
    }

    return glm::perspective(glm::radians(field_of_view_degrees), aspect_ratio, near_clip, far_clip);
}

glm::mat4 Camera3D::GetViewProjectionMatrix() const {
    return GetProjectionMatrix() * GetViewMatrix();
}
