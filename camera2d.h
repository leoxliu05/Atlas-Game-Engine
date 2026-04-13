#ifndef CAMERA2D_H
#define CAMERA2D_H

#include "SDL2/SDL.h"
#include "glm.hpp"

class camera2d {
public:
    inline static camera2d* current_camera2d = nullptr;

    glm::vec2 position = glm::vec2(0.0f, 0.0f);
    float zoom_factor = 1.0f;
    SDL_FRect viewport_rect = { 0.0f, 0.0f, 640.0f, 360.0f };

    void SetPosition(float x, float y)
    {
        position.x = x;
        position.y = y;
    }

    float GetPositionX() const
    {
        return position.x;
    }

    float GetPositionY() const
    {
        return position.y;
    }

    void SetZoom(float zoom)
    {
        zoom_factor = zoom;
    }

    float GetZoom() const
    {
        return zoom_factor;
    }

    void SetViewportSize(float width, float height)
    {
        viewport_rect.w = width;
        viewport_rect.h = height;
    }

    static void SetCameraPosition(float x, float y) { //api 2d
        if (current_camera2d == nullptr)
            return;
        current_camera2d->SetPosition(x, y);
    }

    static float GetCameraPositionX() { //api 2d
        if (current_camera2d == nullptr)
            return 0.0f;
        return current_camera2d->GetPositionX();
    }

    static float GetCameraPositionY() { //api 2d
        if (current_camera2d == nullptr)
            return 0.0f;
        return current_camera2d->GetPositionY();
    }

    static void SetCameraZoom(float zoom_factor_value) { //api 2d
        if (current_camera2d == nullptr)
            return;
        current_camera2d->SetZoom(zoom_factor_value);
    }

    static float GetCameraZoom() { //api 2d
        if (current_camera2d == nullptr)
            return 1.0f;
        return current_camera2d->GetZoom();
    }
};

#endif
