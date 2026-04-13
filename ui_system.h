#ifndef UI_SYSTEM_H
#define UI_SYSTEM_H

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

struct SDL_Texture;
struct _TTF_Font;
typedef struct _TTF_Font TTF_Font;

class ui_system {
public:
    struct image_draw_request {
        std::string image_name = "";
        float x = 0.0f;
        float y = 0.0f;
        bool screen_space = true;
        float scale_x = 1.0f;
        float scale_y = 1.0f;
        float rotation_degrees = 0.0f;
        float pivot_x = 0.0f;
        float pivot_y = 0.0f;
        std::uint8_t r = 255;
        std::uint8_t g = 255;
        std::uint8_t b = 255;
        std::uint8_t a = 255;
        int sorting_order = 0;
    };

    struct text_draw_request {
        std::string content = "";
        int x = 0;
        int y = 0;
        std::string font_name = "";
        int font_size = 16;
        std::uint8_t r = 255;
        std::uint8_t g = 255;
        std::uint8_t b = 255;
        std::uint8_t a = 255;
    };

    struct pixel_draw_request {
        int x = 0;
        int y = 0;
        std::uint8_t r = 255;
        std::uint8_t g = 255;
        std::uint8_t b = 255;
        std::uint8_t a = 255;
    };

    struct sdl_texture_info {
        SDL_Texture* texture = nullptr;
        float w = 0.0f;
        float h = 0.0f;
    };

    struct gl_texture_info {
        unsigned int texture_id = 0;
        float w = 0.0f;
        float h = 0.0f;
    };

    std::vector<image_draw_request> image_requests;
    std::vector<text_draw_request> text_requests;
    std::vector<pixel_draw_request> pixel_requests;

    std::unordered_map<std::string, sdl_texture_info> sdl_image_cache;
    std::unordered_map<std::string, sdl_texture_info> sdl_text_cache;

    std::unordered_map<std::string, gl_texture_info> gl_image_cache;
    std::unordered_map<std::string, gl_texture_info> gl_text_cache;

    std::unordered_map<std::string, std::unordered_map<int, TTF_Font*>> font_cache;
    bool ttf_initialized = false;

    static void DrawImage(const std::string& image_name, float x, float y);
    static void DrawImageEx(const std::string& image_name, float x, float y, float rotation_degrees, float scale_x, float scale_y, float pivot_x, float pivot_y, float r, float g, float b, float a, float sorting_order);
    static void DrawWorldImage(const std::string& image_name, float x, float y);
    static void DrawWorldImageEx(const std::string& image_name, float x, float y, float rotation_degrees, float scale_x, float scale_y, float pivot_x, float pivot_y, float r, float g, float b, float a, float sorting_order);
    static void DrawText(const std::string& content, float x, float y, const std::string& font_name, float font_size, float r, float g, float b, float a);
    static void DrawPixel(float x, float y, float r, float g, float b, float a);

    void reserve();
    void ensure_ttf_initialized();
    TTF_Font* cache_font(const std::string& font_name, int font_size);
    std::string get_image_path(const std::string& image_name) const;
    std::string get_font_path(const std::string& font_name) const;
    std::string get_text_cache_key(const text_draw_request& request) const;
    void submit_image(const std::string& image_name,
                      float x,
                      float y,
                      bool screen_space,
                      float rotation_degrees = 0.0f,
                      float scale_x = 1.0f,
                      float scale_y = 1.0f,
                      float pivot_x = 0.0f,
                      float pivot_y = 0.0f,
                      float r = 255.0f,
                      float g = 255.0f,
                      float b = 255.0f,
                      float a = 255.0f,
                      float sorting_order = 0.0f);
    void submit_ui_text(const std::string& content,
                        float x,
                        float y,
                        const std::string& font_name,
                        float font_size,
                        float r,
                        float g,
                        float b,
                        float a);
    void submit_ui_pixel(float x, float y, float r, float g, float b, float a);
    void clear();
};

#endif // UI_SYSTEM_H
