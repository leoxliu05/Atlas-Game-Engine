#ifndef RENDERER2D_H
#define RENDERER2D_H

#include "glm.hpp"
#include "Helper.h"
#include "ui_system.h"
#include "SDL2_image/SDL_image.h"
#include "SDL2_ttf/SDL_ttf.h"
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

struct SDL_Window;
struct SDL_Renderer;
struct SDL_Texture;

class renderer2d {
public:
    inline static renderer2d* current_renderer = nullptr;
    using ImageInfo = ui_system::sdl_texture_info;

    SDL_Window* window = nullptr;
    SDL_Renderer* my_renderer = nullptr;

    std::uint8_t clear_color_r = 255;
    std::uint8_t clear_color_g = 255;
    std::uint8_t clear_color_b = 255;

    void initialize(const std::string& game_title);
    void render();
    void end();

    static std::unordered_map<std::string, ImageInfo>& GetImageCache();
    ui_system::sdl_texture_info* cache_image(const std::string& image_name);
    ui_system::sdl_texture_info* cache_text(const ui_system::text_draw_request& request);
    void render_ui(ui_system& ui);
};

#endif // !RENDERER2D_H
