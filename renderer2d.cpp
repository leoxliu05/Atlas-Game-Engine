#include "renderer2d.h"
#include "engine.h"
#include <algorithm>

namespace {
bool rects_overlap(const SDL_FRect& a, const SDL_FRect& b) {
    const float a_center_x = a.x + a.w * 0.5f;
    const float a_center_y = a.y + a.h * 0.5f;
    const float b_center_x = b.x + b.w * 0.5f;
    const float b_center_y = b.y + b.h * 0.5f;
    const float a_radius = glm::sqrt(a.w * a.w + a.h * a.h) * 0.5f;
    const float b_radius = glm::sqrt(b.w * b.w + b.h * b.h) * 0.5f;
    const float dx = a_center_x - b_center_x;
    const float dy = a_center_y - b_center_y;
    const float radius_sum = a_radius + b_radius;
    return dx * dx + dy * dy <= radius_sum * radius_sum;
}
}

void renderer2d::initialize(const std::string& game_title) {
    current_renderer = this;
    engine::current_engine->my_camera2d.SetViewportSize(engine::current_engine->camera_w, engine::current_engine->camera_h);
    window = Helper::SDL_CreateWindow(game_title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, static_cast<int>(engine::current_engine->camera_w), static_cast<int>(engine::current_engine->camera_h), 0);
    if (window == nullptr) {
        std::cout << "error: failed to create camera: " << SDL_GetError();
        exit(0);
    }
    my_renderer = Helper::SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
    if (my_renderer == nullptr) {
        std::cout << "error: failed to create renderer: " << SDL_GetError();
        exit(0);
    }
    SDL_SetRenderDrawColor(my_renderer, clear_color_r, clear_color_g, clear_color_b, 255);
    SDL_RenderClear(my_renderer);
    engine::current_engine->my_camera2d.SetPosition(0.0f, 0.0f);
}

void renderer2d::render() {
    if (my_renderer == nullptr)
        return;
    SDL_SetRenderDrawColor(my_renderer, clear_color_r, clear_color_g, clear_color_b, 255);
    SDL_RenderClear(my_renderer);
    SDL_RenderSetScale(my_renderer, 1.0f, 1.0f);
    render_ui(engine::current_engine->my_ui_system);
    Helper::SDL_RenderPresent(my_renderer);
}

void renderer2d::end() {
    ui_system& ui = engine::current_engine->my_ui_system;
    for (auto& entry : ui.sdl_image_cache)
        if (entry.second.texture != nullptr)
            SDL_DestroyTexture(entry.second.texture);
    ui.sdl_image_cache.clear();
    for (auto& entry : ui.sdl_text_cache)
        if (entry.second.texture != nullptr)
            SDL_DestroyTexture(entry.second.texture);
    ui.sdl_text_cache.clear();
    if (my_renderer != nullptr) SDL_DestroyRenderer(my_renderer);
    if (window != nullptr) SDL_DestroyWindow(window);
    current_renderer = nullptr;
}

std::unordered_map<std::string, renderer2d::ImageInfo>& renderer2d::GetImageCache() {
    return engine::current_engine->my_ui_system.sdl_image_cache;
}

ui_system::sdl_texture_info* renderer2d::cache_image(const std::string& image_name) {
    if (image_name.empty())
        return nullptr;
    ui_system& ui = engine::current_engine->my_ui_system;
    auto it = ui.sdl_image_cache.find(image_name);
    if (it != ui.sdl_image_cache.end())
        return &it->second;
    std::string image_path = ui.get_image_path(image_name);
    if (!std::filesystem::exists(image_path)) {
        std::cout << "error: missing image " << image_name;
        exit(0);
    }
    ui_system::sdl_texture_info info;
    info.texture = IMG_LoadTexture(my_renderer, image_path.c_str());
    if (info.texture == nullptr) {
        std::cout << "error: missing image " << image_name;
        exit(0);
    }
    float tex_w = 0.0f;
    float tex_h = 0.0f;
    Helper::SDL_QueryTexture(info.texture, &tex_w, &tex_h);
    info.w = tex_w;
    info.h = tex_h;
    auto insert_result = ui.sdl_image_cache.emplace(image_name, std::move(info));
    return &insert_result.first->second;
}


ui_system::sdl_texture_info* renderer2d::cache_text(const ui_system::text_draw_request& request) {
    ui_system& ui = engine::current_engine->my_ui_system;
    const std::string cache_key = ui.get_text_cache_key(request);
    auto it = ui.sdl_text_cache.find(cache_key);
    if (it != ui.sdl_text_cache.end())
        return &it->second;
    TTF_Font* font = ui.cache_font(request.font_name, request.font_size);
    SDL_Color color = { request.r, request.g, request.b, request.a };
    SDL_Surface* surface = TTF_RenderText_Solid(font, request.content.c_str(), color);
    if (surface == nullptr) {
        std::cout << "error: failed to render text";
        exit(0);
    }
    ui_system::sdl_texture_info info;
    info.texture = SDL_CreateTextureFromSurface(my_renderer, surface);
    if (info.texture == nullptr) {
        SDL_FreeSurface(surface);
        std::cout << "error: failed to render text";
        exit(0);
    }
    info.w = static_cast<float>(surface->w);
    info.h = static_cast<float>(surface->h);
    SDL_FreeSurface(surface);
    auto insert_result = ui.sdl_text_cache.emplace(cache_key, std::move(info));
    return &insert_result.first->second;
}

void renderer2d::render_ui(ui_system& ui) {
    if (ui.image_requests.empty() && ui.text_requests.empty() && ui.pixel_requests.empty())
        return;

    if (ui.image_requests.size() > 1 && !std::is_sorted(ui.image_requests.begin(), ui.image_requests.end(), [](const ui_system::image_draw_request& left, const ui_system::image_draw_request& right) { return left.sorting_order < right.sorting_order; }))
        std::stable_sort(ui.image_requests.begin(), ui.image_requests.end(), [](const ui_system::image_draw_request& left, const ui_system::image_draw_request& right) { return left.sorting_order < right.sorting_order; });
    for (const ui_system::image_draw_request& request : ui.image_requests) {
        ui_system::sdl_texture_info* info = cache_image(request.image_name);
        if (info == nullptr || info->texture == nullptr)
            continue;
        const float base_w = info->w * glm::abs(request.scale_x);
        const float base_h = info->h * glm::abs(request.scale_y);
        SDL_RendererFlip flip = SDL_FLIP_NONE;
        if (request.scale_x < 0.0f)
            flip = static_cast<SDL_RendererFlip>(flip | SDL_FLIP_HORIZONTAL);
        if (request.scale_y < 0.0f)
            flip = static_cast<SDL_RendererFlip>(flip | SDL_FLIP_VERTICAL);
        SDL_SetTextureColorMod(info->texture, request.r, request.g, request.b);
        SDL_SetTextureAlphaMod(info->texture, request.a);
        SDL_FRect dst = { request.x, request.y, base_w, base_h };
        SDL_FPoint pivot_point = { request.pivot_x * base_w, request.pivot_y * base_h };

        if (!request.screen_space) {
            const camera2d& camera = engine::current_engine->my_camera2d;
            const glm::vec2 final_rendering_position = glm::vec2(request.x, request.y) - camera.position;
            const float zoom = camera.zoom_factor;
            const float scaled_w = base_w * zoom;
            const float scaled_h = base_h * zoom;
            dst = {
                final_rendering_position.x * engine::current_engine->pixels_per_unit * zoom + engine::current_engine->camera_w * 0.5f - request.pivot_x * scaled_w,
                final_rendering_position.y * engine::current_engine->pixels_per_unit * zoom + engine::current_engine->camera_h * 0.5f - request.pivot_y * scaled_h,
                scaled_w,
                scaled_h
            };
            pivot_point = { request.pivot_x * scaled_w, request.pivot_y * scaled_h };
            const SDL_FRect viewport = { 0.0f, 0.0f, engine::current_engine->camera_w, engine::current_engine->camera_h };
            if (!rects_overlap(dst, viewport))
                continue;
        }

        Helper::SDL_RenderCopyEx(0, "", my_renderer, info->texture, nullptr, &dst, request.rotation_degrees, &pivot_point, flip);
    }

    for (const ui_system::text_draw_request& request : ui.text_requests) {
        ui_system::sdl_texture_info* text_info = cache_text(request);
        if (text_info == nullptr || text_info->texture == nullptr)
            continue;
        SDL_FRect dst = { static_cast<float>(request.x), static_cast<float>(request.y), text_info->w, text_info->h };
        Helper::SDL_RenderCopyEx(0, "", my_renderer, text_info->texture, nullptr, &dst, 0.0f, nullptr, SDL_FLIP_NONE);
    }

    if (!ui.pixel_requests.empty()) {
        const std::uint8_t current_r = clear_color_r;
        const std::uint8_t current_g = clear_color_g;
        const std::uint8_t current_b = clear_color_b;
        const std::uint8_t current_a = 255;
        SDL_SetRenderDrawBlendMode(my_renderer, SDL_BLENDMODE_BLEND);
        std::vector<SDL_FPoint> point_batch;
        point_batch.reserve(ui.pixel_requests.size());
        std::uint8_t batch_r = ui.pixel_requests.front().r;
        std::uint8_t batch_g = ui.pixel_requests.front().g;
        std::uint8_t batch_b = ui.pixel_requests.front().b;
        std::uint8_t batch_a = ui.pixel_requests.front().a;
        SDL_SetRenderDrawColor(my_renderer, batch_r, batch_g, batch_b, batch_a);
        for (const ui_system::pixel_draw_request& request : ui.pixel_requests) {
            if (request.r != batch_r || request.g != batch_g || request.b != batch_b || request.a != batch_a) {
                if (!point_batch.empty()) {
                    SDL_RenderDrawPointsF(my_renderer, point_batch.data(), static_cast<int>(point_batch.size()));
                    point_batch.clear();
                }
                batch_r = request.r;
                batch_g = request.g;
                batch_b = request.b;
                batch_a = request.a;
                SDL_SetRenderDrawColor(my_renderer, batch_r, batch_g, batch_b, batch_a);
            }
            point_batch.emplace_back(SDL_FPoint{ static_cast<float>(request.x), static_cast<float>(request.y) });
        }
        if (!point_batch.empty())
            SDL_RenderDrawPointsF(my_renderer, point_batch.data(), static_cast<int>(point_batch.size()));
        SDL_SetRenderDrawBlendMode(my_renderer, SDL_BLENDMODE_NONE);
        SDL_SetRenderDrawColor(my_renderer, current_r, current_g, current_b, current_a);
    }

    ui.clear();
}
