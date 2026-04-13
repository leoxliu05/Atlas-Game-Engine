#ifndef RENDERER3D_H
#define RENDERER3D_H

#include "camera3d.h"
#include "directional_light3d.h"
#include "mesh3d.h"
#include "transform3d.h"
#include "ui_system.h"
#include "glm.hpp"
#include "SDL2/SDL.h"
#include "SDL2_ttf/SDL_ttf.h"
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

struct Material3D {
    std::string image_name = "";
    glm::vec3 tint = glm::vec3(1.0f);
    float alpha = 1.0f;
    bool unlit = false;
};

class renderer3d {
public:
    inline static renderer3d* current_renderer = nullptr;

    struct mesh_draw_request {
        Mesh3D mesh;
        Transform3D transform;
        Camera3D camera;
        DirectionalLight3D light;
        Material3D material;
    };

    struct material_texture_info {
        unsigned int texture_id = 0;
        float w = 0.0f;
        float h = 0.0f;
    };

    SDL_Window* window = nullptr;
    SDL_GLContext gl_context = nullptr;
    unsigned int shader_program = 0;
    unsigned int vertex_array_object = 0;
    unsigned int vertex_buffer_object = 0;
    unsigned int element_buffer_object = 0;

    unsigned int ui_shader_program = 0;
    unsigned int ui_vertex_array_object = 0;
    unsigned int ui_vertex_buffer_object = 0;
    unsigned int ui_element_buffer_object = 0;
    unsigned int ui_white_texture = 0;

    glm::vec4 clear_color = glm::vec4(0.08f, 0.08f, 0.10f, 1.0f);
    std::vector<mesh_draw_request> mesh_requests;
    std::unordered_map<std::string, material_texture_info> material_texture_cache;

    void initialize(const std::string& game_title, int width, int height);
    void submit_mesh(const Mesh3D& mesh, const Transform3D& transform, const Camera3D& camera, const DirectionalLight3D& light, const Material3D& material = Material3D());
    void submit_mesh_simple(const Mesh3D& mesh,
                            const glm::vec3& mesh_position,
                            const glm::vec3& mesh_rotation_degrees,
                            const glm::vec3& camera_position,
                            const glm::vec3& camera_rotation_degrees,
                            const DirectionalLight3D& light,
                            const Material3D& material = Material3D());
    void set_clear_color(float r, float g, float b, float a);
    void render();
    void end();
    void clear_queue();

private:
    material_texture_info* cache_material_texture(const Material3D& material);
    ui_system::gl_texture_info* cache_ui_image(const std::string& image_name);
    ui_system::gl_texture_info* cache_ui_text(const ui_system::text_draw_request& request);
    void render_ui(ui_system& ui);
};

#endif // RENDERER3D_H
