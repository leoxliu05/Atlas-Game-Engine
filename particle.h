#ifndef PARTICLE_H
#define PARTICLE_H

#include "renderer2d.h"
#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <string>
#include <vector>

inline constexpr const char* default_particle_texture_name = "__default_particle_texture__";

class ParticleSystem {
public:
    actor* owner = nullptr;
    std::string key = "";
    bool enabled = true;

    float x = 0.0f;
    float y = 0.0f;
    float emit_angle_min = 0.0f;
    float emit_angle_max = 360.0f;
    float emit_radius_min = 0.0f;
    float emit_radius_max = 0.5f;
    float start_scale_min = 1.0f;
    float start_scale_max = 1.0f;
    float start_speed_min = 0.0f;
    float start_speed_max = 0.0f;
    float rotation_min = 0.0f;
    float rotation_max = 0.0f;
    float rotation_speed_min = 0.0f;
    float rotation_speed_max = 0.0f;
    float gravity_scale_x = 0.0f;
    float gravity_scale_y = 0.0f;
    float drag_factor = 1.0f;
    float angular_drag_factor = 1.0f;
    float end_scale = std::numeric_limits<float>::quiet_NaN();
    int start_color_r = 255;
    int start_color_g = 255;
    int start_color_b = 255;
    int start_color_a = 255;
    int end_color_r = -1;
    int end_color_g = -1;
    int end_color_b = -1;
    int end_color_a = -1;
    std::string image = "";
    int sorting_order = 9999;
    int frames_between_bursts = 1;
    int burst_quantity = 1;
    int duration_frames = 300;
    int local_frame_number = 0;
    bool is_playing = true;

    std::vector<float> xs;
    std::vector<float> ys;
    std::vector<float> velocity_xs;
    std::vector<float> velocity_ys;
    std::vector<float> initial_scales;
    std::vector<float> scales;
    std::vector<float> rotations;
    std::vector<float> rotation_speeds;
    std::vector<std::uint8_t> initial_rs;
    std::vector<std::uint8_t> initial_gs;
    std::vector<std::uint8_t> initial_bs;
    std::vector<std::uint8_t> initial_as;
    std::vector<std::uint8_t> rs;
    std::vector<std::uint8_t> gs;
    std::vector<std::uint8_t> bs;
    std::vector<std::uint8_t> as;
    std::vector<int> frames_alive;
    std::vector<std::uint8_t> is_active;
    std::vector<int> free_list;
    std::unique_ptr<RandomEngine> emit_angle_distribution;
    std::unique_ptr<RandomEngine> emit_radius_distribution;
    std::unique_ptr<RandomEngine> scale_distribution;
    std::unique_ptr<RandomEngine> speed_distribution;
    std::unique_ptr<RandomEngine> rotation_distribution;
    std::unique_ptr<RandomEngine> rotation_speed_distribution;
    renderer2d::ImageInfo* particle_image = nullptr;

    void OnStart() {
        auto& image_cache = renderer2d::GetImageCache();
        if (image_cache.find(default_particle_texture_name) == image_cache.end()) {
            SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(0, 8, 8, 32, SDL_PIXELFORMAT_RGBA8888);
            SDL_FillRect(surface, nullptr, SDL_MapRGBA(surface->format, 255, 255, 255, 255));
            SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer2d::current_renderer->my_renderer, surface);
            SDL_FreeSurface(surface);
            image_cache[default_particle_texture_name] = renderer2d::ImageInfo{ texture, 8.0f, 8.0f };
        }

        emit_angle_distribution = std::make_unique<RandomEngine>(emit_angle_min, emit_angle_max, 298);
        emit_radius_distribution = std::make_unique<RandomEngine>(emit_radius_min, emit_radius_max, 404);
        scale_distribution = std::make_unique<RandomEngine>(start_scale_min, start_scale_max, 494);
        speed_distribution = std::make_unique<RandomEngine>(start_speed_min, start_speed_max, 498);
        rotation_distribution = std::make_unique<RandomEngine>(rotation_min, rotation_max, 440);
        rotation_speed_distribution = std::make_unique<RandomEngine>(rotation_speed_min, rotation_speed_max, 305);
        particle_image = renderer2d::current_renderer->cache_image(image.empty() ? default_particle_texture_name : image);

        const int burst_interval = frames_between_bursts < 1 ? 1 : frames_between_bursts;
        const int particle_lifetime = duration_frames < 1 ? 1 : duration_frames;
        const int burst_count = burst_quantity < 1 ? 1 : burst_quantity;
        reserve_particle_storage(static_cast<std::size_t>(burst_count * (particle_lifetime / burst_interval + 2)));
    }

    void OnUpdate() {
        if (is_playing && local_frame_number % frames_between_bursts == 0)
            Burst();

        const bool has_end_scale = !std::isnan(end_scale);
        const bool has_end_color_r = end_color_r >= 0;
        const bool has_end_color_g = end_color_g >= 0;
        const bool has_end_color_b = end_color_b >= 0;
        const bool has_end_color_a = end_color_a >= 0;

        for (int i = 0; i < static_cast<int>(is_active.size()); ++i) {
            if (!is_active[i])
                continue;
            if (frames_alive[i] >= duration_frames) {
                is_active[i] = 0;
                free_list.push_back(i);
                continue;
            }
            velocity_xs[i] += gravity_scale_x;
            velocity_ys[i] += gravity_scale_y;
            velocity_xs[i] *= drag_factor;
            velocity_ys[i] *= drag_factor;
            rotation_speeds[i] *= angular_drag_factor;
            xs[i] += velocity_xs[i];
            ys[i] += velocity_ys[i];
            rotations[i] += rotation_speeds[i];
            const float lifetime_progress = static_cast<float>(frames_alive[i]) / duration_frames;
            if (has_end_scale)
                scales[i] = glm::mix(initial_scales[i], end_scale, lifetime_progress);
            if (has_end_color_r)
                rs[i] = static_cast<std::uint8_t>(glm::mix(static_cast<float>(initial_rs[i]), static_cast<float>(end_color_r), lifetime_progress));
            if (has_end_color_g)
                gs[i] = static_cast<std::uint8_t>(glm::mix(static_cast<float>(initial_gs[i]), static_cast<float>(end_color_g), lifetime_progress));
            if (has_end_color_b)
                bs[i] = static_cast<std::uint8_t>(glm::mix(static_cast<float>(initial_bs[i]), static_cast<float>(end_color_b), lifetime_progress));
            if (has_end_color_a)
                as[i] = static_cast<std::uint8_t>(glm::mix(static_cast<float>(initial_as[i]), static_cast<float>(end_color_a), lifetime_progress));
            ui_system::DrawWorldImageEx(image.empty() ? default_particle_texture_name : image,
                                        xs[i],
                                        ys[i],
                                        rotations[i],
                                        scales[i],
                                        scales[i],
                                        0.5f,
                                        0.5f,
                                        static_cast<float>(rs[i]),
                                        static_cast<float>(gs[i]),
                                        static_cast<float>(bs[i]),
                                        static_cast<float>(as[i]),
                                        static_cast<float>(sorting_order));
            frames_alive[i]++;
        }

        local_frame_number++;
    }

    void Stop() { // api 2d
        is_playing = false;
    }

    void Play() { // api 2d
        is_playing = true;
    }

    void Burst() { // api 2d
        for (int i = 0; i < burst_quantity; ++i) {
            const float angle_degrees = emit_angle_distribution->Sample();
            const float angle_radians = glm::radians(angle_degrees);
            const float radius = emit_radius_distribution->Sample();
            const float speed = speed_distribution->Sample();
            const int slot = acquire_particle_slot();

            xs[slot] = x + glm::cos(angle_radians) * radius;
            ys[slot] = y + glm::sin(angle_radians) * radius;
            velocity_xs[slot] = glm::cos(angle_radians) * speed;
            velocity_ys[slot] = glm::sin(angle_radians) * speed;
            initial_scales[slot] = scale_distribution->Sample();
            scales[slot] = initial_scales[slot];
            rotations[slot] = rotation_distribution->Sample();
            rotation_speeds[slot] = rotation_speed_distribution->Sample();
            initial_rs[slot] = static_cast<std::uint8_t>(start_color_r);
            initial_gs[slot] = static_cast<std::uint8_t>(start_color_g);
            initial_bs[slot] = static_cast<std::uint8_t>(start_color_b);
            initial_as[slot] = static_cast<std::uint8_t>(start_color_a);
            rs[slot] = initial_rs[slot];
            gs[slot] = initial_gs[slot];
            bs[slot] = initial_bs[slot];
            as[slot] = initial_as[slot];
            frames_alive[slot] = 0;
            is_active[slot] = 1;
        }
    }

    void OnDestroy() {
        xs.clear();
        ys.clear();
        velocity_xs.clear();
        velocity_ys.clear();
        initial_scales.clear();
        scales.clear();
        rotations.clear();
        rotation_speeds.clear();
        initial_rs.clear();
        initial_gs.clear();
        initial_bs.clear();
        initial_as.clear();
        rs.clear();
        gs.clear();
        bs.clear();
        as.clear();
        frames_alive.clear();
        is_active.clear();
        free_list.clear();
        particle_image = nullptr;
    }

private:
    void reserve_particle_storage(std::size_t count) {
        xs.reserve(count);
        ys.reserve(count);
        velocity_xs.reserve(count);
        velocity_ys.reserve(count);
        initial_scales.reserve(count);
        scales.reserve(count);
        rotations.reserve(count);
        rotation_speeds.reserve(count);
        initial_rs.reserve(count);
        initial_gs.reserve(count);
        initial_bs.reserve(count);
        initial_as.reserve(count);
        rs.reserve(count);
        gs.reserve(count);
        bs.reserve(count);
        as.reserve(count);
        frames_alive.reserve(count);
        is_active.reserve(count);
    }

    int acquire_particle_slot() {
        if (!free_list.empty()) {
            const int slot = free_list.back();
            free_list.pop_back();
            return slot;
        }

        const int slot = static_cast<int>(is_active.size());
        xs.emplace_back(0.0f);
        ys.emplace_back(0.0f);
        velocity_xs.emplace_back(0.0f);
        velocity_ys.emplace_back(0.0f);
        initial_scales.emplace_back(1.0f);
        scales.emplace_back(1.0f);
        rotations.emplace_back(0.0f);
        rotation_speeds.emplace_back(0.0f);
        initial_rs.emplace_back(255);
        initial_gs.emplace_back(255);
        initial_bs.emplace_back(255);
        initial_as.emplace_back(255);
        rs.emplace_back(255);
        gs.emplace_back(255);
        bs.emplace_back(255);
        as.emplace_back(255);
        frames_alive.emplace_back(0);
        is_active.emplace_back(0);
        return slot;
    }
};

#endif // !PARTICLE_H
