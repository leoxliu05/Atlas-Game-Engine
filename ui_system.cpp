#include "ui_system.h"
#include "engine.h"
#include "SDL2_ttf/SDL_ttf.h"
#include <filesystem>
#include <iostream>

void ui_system::DrawImage(const std::string& image_name, float x, float y) {
    if (engine::current_engine == nullptr)
        return;
    engine::current_engine->my_ui_system.submit_image(image_name, x, y, true);
}

void ui_system::DrawImageEx(const std::string& image_name,
                            float x,
                            float y,
                            float rotation_degrees,
                            float scale_x,
                            float scale_y,
                            float pivot_x,
                            float pivot_y,
                            float r,
                            float g,
                            float b,
                            float a,
                            float sorting_order) {
    if (engine::current_engine == nullptr)
        return;
    engine::current_engine->my_ui_system.submit_image(image_name, x, y, true, rotation_degrees, scale_x, scale_y, pivot_x, pivot_y, r, g, b, a, sorting_order);
}

void ui_system::DrawWorldImage(const std::string& image_name, float x, float y) {
    if (engine::current_engine == nullptr)
        return;
    engine::current_engine->my_ui_system.submit_image(image_name, x, y, false, 0.0f, 1.0f, 1.0f, 0.5f, 0.5f);
}

void ui_system::DrawWorldImageEx(const std::string& image_name,
                                 float x,
                                 float y,
                                 float rotation_degrees,
                                 float scale_x,
                                 float scale_y,
                                 float pivot_x,
                                 float pivot_y,
                                 float r,
                                 float g,
                                 float b,
                                 float a,
                                 float sorting_order) {
    if (engine::current_engine == nullptr)
        return;
    engine::current_engine->my_ui_system.submit_image(image_name, x, y, false, rotation_degrees, scale_x, scale_y, pivot_x, pivot_y, r, g, b, a, sorting_order);
}

void ui_system::DrawText(const std::string& content,
                         float x,
                         float y,
                         const std::string& font_name,
                         float font_size,
                         float r,
                         float g,
                         float b,
                         float a) {
    if (engine::current_engine == nullptr)
        return;
    engine::current_engine->my_ui_system.submit_ui_text(content, x, y, font_name, font_size, r, g, b, a);
}

void ui_system::DrawPixel(float x, float y, float r, float g, float b, float a) {
    if (engine::current_engine == nullptr)
        return;
    engine::current_engine->my_ui_system.submit_ui_pixel(x, y, r, g, b, a);
}

void ui_system::reserve() {
    image_requests.reserve(1024);
    text_requests.reserve(256);
    pixel_requests.reserve(4096);
}

void ui_system::ensure_ttf_initialized() {
    if (ttf_initialized)
        return;
    if (TTF_Init() != 0) {
        std::cout << "error: TTF_Init failed: " << TTF_GetError();
        exit(0);
    }
    ttf_initialized = true;
}

TTF_Font* ui_system::cache_font(const std::string& font_name, int font_size) {
    if (font_name.empty()) {
        std::cout << "error: text render failed. No font configured";
        exit(0);
    }

    auto& size_map = font_cache[font_name];
    auto it = size_map.find(font_size);
    if (it != size_map.end())
        return it->second;

    std::string font_path = get_font_path(font_name);
    if (!std::filesystem::exists(font_path)) {
        std::cout << "error: font " << font_name << " missing";
        exit(0);
    }

    ensure_ttf_initialized();
    TTF_Font* loaded_font = TTF_OpenFont(font_path.c_str(), font_size);
    if (loaded_font == nullptr) {
        std::cout << "error: font " << font_name << " missing";
        exit(0);
    }

    size_map.emplace(font_size, loaded_font);
    return loaded_font;
}

std::string ui_system::get_image_path(const std::string& image_name) const {
    std::string image_path = "resources/images/" + image_name;
    if (image_path.size() < 4 || image_path.substr(image_path.size() - 4) != ".png")
        image_path += ".png";
    return image_path;
}

std::string ui_system::get_font_path(const std::string& font_name) const {
    std::string font_path = "resources/fonts/" + font_name;
    if (font_path.size() < 4 || font_path.substr(font_path.size() - 4) != ".ttf")
        font_path += ".ttf";
    return font_path;
}

std::string ui_system::get_text_cache_key(const text_draw_request& request) const {
    return request.font_name + '\n'
        + std::to_string(request.font_size) + '\n'
        + std::to_string(request.r) + '\n'
        + std::to_string(request.g) + '\n'
        + std::to_string(request.b) + '\n'
        + std::to_string(request.a) + '\n'
        + request.content;
}

void ui_system::submit_image(const std::string& image_name,
                             float x,
                             float y,
                             bool screen_space,
                             float rotation_degrees,
                             float scale_x,
                             float scale_y,
                             float pivot_x,
                             float pivot_y,
                             float r,
                             float g,
                             float b,
                             float a,
                             float sorting_order) {
    image_requests.emplace_back(image_draw_request{
        image_name,
        x,
        y,
        screen_space,
        scale_x,
        scale_y,
        rotation_degrees,
        pivot_x,
        pivot_y,
        static_cast<std::uint8_t>(static_cast<int>(r)),
        static_cast<std::uint8_t>(static_cast<int>(g)),
        static_cast<std::uint8_t>(static_cast<int>(b)),
        static_cast<std::uint8_t>(static_cast<int>(a)),
        static_cast<int>(sorting_order)
    });
}

void ui_system::submit_ui_text(const std::string& content,
                               float x,
                               float y,
                               const std::string& font_name,
                               float font_size,
                               float r,
                               float g,
                               float b,
                               float a) {
    if (content.empty())
        return;

    text_requests.emplace_back(text_draw_request{
        content,
        static_cast<int>(x),
        static_cast<int>(y),
        font_name,
        static_cast<int>(font_size),
        static_cast<std::uint8_t>(static_cast<int>(r)),
        static_cast<std::uint8_t>(static_cast<int>(g)),
        static_cast<std::uint8_t>(static_cast<int>(b)),
        static_cast<std::uint8_t>(static_cast<int>(a))
    });
}

void ui_system::submit_ui_pixel(float x, float y, float r, float g, float b, float a) {
    pixel_requests.emplace_back(pixel_draw_request{
        static_cast<int>(x),
        static_cast<int>(y),
        static_cast<std::uint8_t>(static_cast<int>(r)),
        static_cast<std::uint8_t>(static_cast<int>(g)),
        static_cast<std::uint8_t>(static_cast<int>(b)),
        static_cast<std::uint8_t>(static_cast<int>(a))
    });
}

void ui_system::clear() {
    image_requests.clear();
    text_requests.clear();
    pixel_requests.clear();
}
