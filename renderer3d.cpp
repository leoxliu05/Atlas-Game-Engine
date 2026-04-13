#include "renderer3d.h"
#include "Helper.h"
#include "engine.h"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <filesystem>
#include <iostream>
#include <utility>
#include <vector>

#if defined(__APPLE__)
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl3.h>
#else
#define GL_GLEXT_PROTOTYPES
#include "SDL2/SDL_opengl.h"
#include "SDL2/SDL_opengl_glext.h"
#endif

namespace {

struct GpuVertex3D {
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec2 uv = glm::vec2(0.0f);
    glm::vec4 color = glm::vec4(1.0f);
};

struct GpuUiVertex {
    glm::vec2 position = glm::vec2(0.0f);
    glm::vec2 uv = glm::vec2(0.0f);
    glm::vec4 color = glm::vec4(1.0f);
};

const char* vertex_shader_source = R"(#version 150 core
in vec3 aPosition;
in vec2 aUV;
in vec4 aTint;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

out vec2 vUV;
out vec4 vTint;

void main() {
    vUV = aUV;
    vTint = aTint;
    gl_Position = uProjection * uView * uModel * vec4(aPosition, 1.0);
}
)";

const char* fragment_shader_source = R"(#version 150 core
in vec2 vUV;
in vec4 vTint;

uniform sampler2D uTexture;
out vec4 fragColor;

void main() {
    fragColor = texture(uTexture, vUV) * vTint;
}
)";

const char* ui_vertex_shader_source = R"(#version 150 core
in vec2 aPosition;
in vec2 aUV;
in vec4 aColor;

uniform mat4 uProjection;

out vec2 vUV;
out vec4 vColor;

void main() {
    vUV = aUV;
    vColor = aColor;
    gl_Position = uProjection * vec4(aPosition, 0.0, 1.0);
}
)";

const char* ui_fragment_shader_source = R"(#version 150 core
in vec2 vUV;
in vec4 vColor;

uniform sampler2D uTexture;

out vec4 fragColor;

void main() {
    fragColor = texture(uTexture, vUV) * vColor;
}
)";

glm::vec3 compute_world_position(const glm::mat4& model, const glm::vec3& local_position) {
    return glm::vec3(model * glm::vec4(local_position, 1.0f));
}

glm::vec4 clamp_color(const glm::vec4& color) {
    return glm::vec4(
        glm::clamp(color.r, 0.0f, 1.0f),
        glm::clamp(color.g, 0.0f, 1.0f),
        glm::clamp(color.b, 0.0f, 1.0f),
        glm::clamp(color.a, 0.0f, 1.0f)
    );
}

glm::vec3 compute_face_normal(const glm::vec3& world_p0,
                              const glm::vec3& world_p1,
                              const glm::vec3& world_p2) {
    const glm::vec3 edge_a = world_p1 - world_p0;
    const glm::vec3 edge_b = world_p2 - world_p0;
    glm::vec3 normal = glm::cross(edge_b, edge_a);
    if (glm::length(normal) <= 0.00001f)
        return glm::vec3(0.0f, 1.0f, 0.0f);
    return glm::normalize(normal);
}

glm::vec3 orient_normal_outward(const glm::vec3& normal,
                                const glm::vec3& face_center,
                                const glm::vec3& model_center) {
    if (glm::dot(normal, face_center - model_center) < 0.0f)
        return -normal;
    return normal;
}

glm::vec4 apply_lighting(const glm::vec4& tint_color,
                         const glm::vec3& normal,
                         const glm::vec3& sun_direction,
                         const glm::vec3& sun_color,
                         bool unlit) {
    if (unlit)
        return tint_color;

    const float sun = glm::max(glm::dot(normal, sun_direction), 0.0f);
    const float ambient = 0.18f;
    const glm::vec3 light = glm::vec3(ambient) + sun_color * (sun * 0.82f);

    return clamp_color(glm::vec4(
        tint_color.r * light.r,
        tint_color.g * light.g,
        tint_color.b * light.b,
        tint_color.a
    ));
}

bool check_shader_status(GLuint shader, GLenum status_flag, const std::string& label) {
    GLint status = GL_FALSE;
    glGetShaderiv(shader, status_flag, &status);
    if (status == GL_TRUE)
        return true;

    GLint info_log_length = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &info_log_length);
    std::vector<char> info_log(static_cast<std::size_t>(glm::max(info_log_length, 1)), '\0');
    glGetShaderInfoLog(shader, info_log_length, nullptr, info_log.data());
    std::cout << "error: failed to compile " << label << ": " << info_log.data();
    return false;
}

bool check_program_status(GLuint program, GLenum status_flag, const std::string& label) {
    GLint status = GL_FALSE;
    glGetProgramiv(program, status_flag, &status);
    if (status == GL_TRUE)
        return true;

    GLint info_log_length = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &info_log_length);
    std::vector<char> info_log(static_cast<std::size_t>(glm::max(info_log_length, 1)), '\0');
    glGetProgramInfoLog(program, info_log_length, nullptr, info_log.data());
    std::cout << "error: failed to link " << label << ": " << info_log.data();
    return false;
}

GLuint compile_shader(GLenum shader_type, const char* source, const std::string& label) {
    const GLuint shader = glCreateShader(shader_type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    if (!check_shader_status(shader, GL_COMPILE_STATUS, label)) {
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

GLuint create_shader_program(const char* vertex_source,
                             const char* fragment_source,
                             const std::string& label,
                             const std::vector<std::pair<GLuint, std::string>>& attributes) {
    const GLuint vertex_shader = compile_shader(GL_VERTEX_SHADER, vertex_source, label + " vertex shader");
    if (vertex_shader == 0)
        return 0;

    const GLuint fragment_shader = compile_shader(GL_FRAGMENT_SHADER, fragment_source, label + " fragment shader");
    if (fragment_shader == 0) {
        glDeleteShader(vertex_shader);
        return 0;
    }

    const GLuint program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    for (const auto& attribute : attributes)
        glBindAttribLocation(program, attribute.first, attribute.second.c_str());
    glLinkProgram(program);

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    if (!check_program_status(program, GL_LINK_STATUS, label + " program")) {
        glDeleteProgram(program);
        return 0;
    }

    return program;
}

GLuint create_world_shader_program() {
    return create_shader_program(
        vertex_shader_source,
        fragment_shader_source,
        "3D shader",
        { { 0, "aPosition" }, { 1, "aUV" }, { 2, "aTint" } }
    );
}

GLuint create_ui_shader_program() {
    return create_shader_program(
        ui_vertex_shader_source,
        ui_fragment_shader_source,
        "UI shader",
        { { 0, "aPosition" }, { 1, "aUV" }, { 2, "aColor" } }
    );
}

void build_gpu_buffers(const renderer3d::mesh_draw_request& request,
                       std::vector<GpuVertex3D>& out_vertices,
                       std::vector<std::uint32_t>& out_indices) {
    out_vertices.clear();
    out_indices.clear();

    if (!request.mesh.HasVertices() || !request.mesh.HasIndices())
        return;

    out_vertices.reserve(request.mesh.indices.size());
    out_indices.reserve(request.mesh.indices.size());

    const glm::mat4 model = request.transform.GetModelMatrix();
    const glm::vec3 model_center = compute_world_position(model, glm::vec3(0.0f));
    const glm::vec3 light_direction = request.light.GetDirection();
    const glm::vec3 light_radiance = request.light.GetRadiance();

    for (std::size_t i = 0; i + 2 < request.mesh.indices.size(); i += 3) {
        const std::uint32_t i0 = request.mesh.indices[i];
        const std::uint32_t i1 = request.mesh.indices[i + 1];
        const std::uint32_t i2 = request.mesh.indices[i + 2];
        if (i0 >= request.mesh.vertices.size() || i1 >= request.mesh.vertices.size() || i2 >= request.mesh.vertices.size())
            continue;

        const Vertex3D& v0 = request.mesh.vertices[i0];
        const Vertex3D& v1 = request.mesh.vertices[i1];
        const Vertex3D& v2 = request.mesh.vertices[i2];

        const glm::vec3 world_p0 = compute_world_position(model, v0.position);
        const glm::vec3 world_p1 = compute_world_position(model, v1.position);
        const glm::vec3 world_p2 = compute_world_position(model, v2.position);
        const glm::vec3 face_center = (world_p0 + world_p1 + world_p2) / 3.0f;
        const glm::vec3 normal = orient_normal_outward(
            compute_face_normal(world_p0, world_p1, world_p2),
            face_center,
            model_center
        );

        const glm::vec4 face_color = apply_lighting(
            glm::vec4(request.material.tint, request.material.alpha),
            normal,
            light_direction,
            light_radiance,
            request.material.unlit
        );

        const std::uint32_t base_index = static_cast<std::uint32_t>(out_vertices.size());
        out_vertices.push_back(GpuVertex3D{ v0.position, v0.uv, face_color });
        out_vertices.push_back(GpuVertex3D{ v1.position, v1.uv, face_color });
        out_vertices.push_back(GpuVertex3D{ v2.position, v2.uv, face_color });
        out_indices.push_back(base_index);
        out_indices.push_back(base_index + 1);
        out_indices.push_back(base_index + 2);
    }
}

GLuint create_surface_texture(SDL_Surface* surface, bool nearest_filter) {
    if (surface == nullptr)
        return 0;

    SDL_Surface* rgba_surface = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA32, 0);
    if (rgba_surface == nullptr)
        return 0;

    GLuint texture_id = 0;
    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, nearest_filter ? GL_NEAREST : GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, nearest_filter ? GL_NEAREST : GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RGBA,
                 rgba_surface->w,
                 rgba_surface->h,
                 0,
                 GL_RGBA,
                 GL_UNSIGNED_BYTE,
                 rgba_surface->pixels);
    glBindTexture(GL_TEXTURE_2D, 0);
    SDL_FreeSurface(rgba_surface);
    return texture_id;
}

void fill_ui_quad(std::vector<GpuUiVertex>& out_vertices,
                  float x,
                  float y,
                  float width,
                  float height,
                  float pivot_x,
                  float pivot_y,
                  float rotation_degrees,
                  const glm::vec4& color) {
    const float left = -pivot_x * width;
    const float right = left + width;
    const float top = -pivot_y * height;
    const float bottom = top + height;

    const float radians = glm::radians(rotation_degrees);
    const float cosine = std::cos(radians);
    const float sine = std::sin(radians);

    const glm::vec2 corners[4] = {
        glm::vec2(left, top),
        glm::vec2(right, top),
        glm::vec2(right, bottom),
        glm::vec2(left, bottom)
    };
    const glm::vec2 uvs[4] = {
        glm::vec2(0.0f, 0.0f),
        glm::vec2(1.0f, 0.0f),
        glm::vec2(1.0f, 1.0f),
        glm::vec2(0.0f, 1.0f)
    };

    out_vertices.resize(4);
    for (int i = 0; i < 4; ++i) {
        const glm::vec2 local = corners[i];
        const glm::vec2 rotated(
            local.x * cosine - local.y * sine,
            local.x * sine + local.y * cosine
        );
        out_vertices[i].position = glm::vec2(x, y) + rotated;
        out_vertices[i].uv = uvs[i];
        out_vertices[i].color = color;
    }
}

}

void renderer3d::initialize(const std::string& game_title, int width, int height) {
    current_renderer = this;
    mesh_requests.reserve(1024);

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    window = Helper::SDL_CreateWindow(game_title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    if (window == nullptr) {
        std::cout << "error: failed to create 3D window: " << SDL_GetError();
        exit(0);
    }

    gl_context = SDL_GL_CreateContext(window);
    if (gl_context == nullptr) {
        std::cout << "error: failed to create OpenGL context: " << SDL_GetError();
        exit(0);
    }

    if (SDL_GL_MakeCurrent(window, gl_context) != 0) {
        std::cout << "error: failed to make OpenGL context current: " << SDL_GetError();
        exit(0);
    }

    SDL_GL_SetSwapInterval(1);

    shader_program = create_world_shader_program();
    if (shader_program == 0)
        exit(0);

    ui_shader_program = create_ui_shader_program();
    if (ui_shader_program == 0)
        exit(0);

    glGenVertexArrays(1, &vertex_array_object);
    glGenBuffers(1, &vertex_buffer_object);
    glGenBuffers(1, &element_buffer_object);

    glBindVertexArray(vertex_array_object);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_object);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, element_buffer_object);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GpuVertex3D), reinterpret_cast<const void*>(offsetof(GpuVertex3D, position)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(GpuVertex3D), reinterpret_cast<const void*>(offsetof(GpuVertex3D, uv)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(GpuVertex3D), reinterpret_cast<const void*>(offsetof(GpuVertex3D, color)));
    glBindVertexArray(0);

    glGenVertexArrays(1, &ui_vertex_array_object);
    glGenBuffers(1, &ui_vertex_buffer_object);
    glGenBuffers(1, &ui_element_buffer_object);

    const std::uint32_t ui_indices[] = { 0, 1, 2, 0, 2, 3 };
    glBindVertexArray(ui_vertex_array_object);
    glBindBuffer(GL_ARRAY_BUFFER, ui_vertex_buffer_object);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(4 * sizeof(GpuUiVertex)), nullptr, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ui_element_buffer_object);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(sizeof(ui_indices)), ui_indices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(GpuUiVertex), reinterpret_cast<const void*>(offsetof(GpuUiVertex, position)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(GpuUiVertex), reinterpret_cast<const void*>(offsetof(GpuUiVertex, uv)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(GpuUiVertex), reinterpret_cast<const void*>(offsetof(GpuUiVertex, color)));
    glBindVertexArray(0);

    glGenTextures(1, &ui_white_texture);
    glBindTexture(GL_TEXTURE_2D, ui_white_texture);
    const std::uint8_t white_pixel[] = { 255, 255, 255, 255 };
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white_pixel);
    glBindTexture(GL_TEXTURE_2D, 0);

    glViewport(0, 0, width, height);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glClearColor(clear_color.r, clear_color.g, clear_color.b, clear_color.a);
}

void renderer3d::submit_mesh(const Mesh3D& mesh,
                             const Transform3D& transform,
                             const Camera3D& camera,
                             const DirectionalLight3D& light,
                             const Material3D& material) {
    mesh_draw_request request;
    request.mesh = mesh;
    request.transform = transform;
    request.camera = camera;
    request.light = light;
    request.material = material;
    mesh_requests.emplace_back(std::move(request));
}

void renderer3d::submit_mesh_simple(const Mesh3D& mesh,
                                    const glm::vec3& mesh_position,
                                    const glm::vec3& mesh_rotation_degrees,
                                    const glm::vec3& camera_position,
                                    const glm::vec3& camera_rotation_degrees,
                                    const DirectionalLight3D& light,
                                    const Material3D& material) {
    Transform3D transform;
    transform.position = mesh_position;
    transform.rotation_degrees = mesh_rotation_degrees;
    transform.scale = glm::vec3(1.0f);

    int window_width = 1280;
    int window_height = 720;
    if (window != nullptr)
        SDL_GetWindowSize(window, &window_width, &window_height);

    Camera3D camera;
    camera.transform.position = camera_position;
    camera.transform.rotation_degrees = camera_rotation_degrees;
    camera.field_of_view_degrees = 60.0f;
    camera.aspect_ratio = window_height == 0 ? 16.0f / 9.0f : static_cast<float>(window_width) / static_cast<float>(window_height);
    camera.near_clip = 0.1f;
    camera.far_clip = 100.0f;

    submit_mesh(mesh, transform, camera, light, material);
}

renderer3d::material_texture_info* renderer3d::cache_material_texture(const Material3D& material) {
    if (material.image_name.empty())
        return nullptr;

    auto it = material_texture_cache.find(material.image_name);
    if (it != material_texture_cache.end())
        return &it->second;

    std::string image_path = engine::current_engine->my_ui_system.get_image_path(material.image_name);
    if (!std::filesystem::exists(image_path)) {
        std::cout << "error: missing image " << material.image_name;
        exit(0);
    }

    SDL_Surface* surface = IMG_Load(image_path.c_str());
    if (surface == nullptr) {
        std::cout << "error: failed to load image " << material.image_name;
        exit(0);
    }

    material_texture_info info;
    info.texture_id = create_surface_texture(surface, true);
    info.w = static_cast<float>(surface->w);
    info.h = static_cast<float>(surface->h);
    SDL_FreeSurface(surface);

    if (info.texture_id == 0) {
        std::cout << "error: failed to upload image " << material.image_name;
        exit(0);
    }

    auto insert_result = material_texture_cache.emplace(material.image_name, std::move(info));
    return &insert_result.first->second;
}

ui_system::gl_texture_info* renderer3d::cache_ui_image(const std::string& image_name) {
    if (image_name.empty())
        return nullptr;

    ui_system& ui = engine::current_engine->my_ui_system;
    auto it = ui.gl_image_cache.find(image_name);
    if (it != ui.gl_image_cache.end())
        return &it->second;

    std::string image_path = ui.get_image_path(image_name);
    if (!std::filesystem::exists(image_path)) {
        std::cout << "error: missing image " << image_name;
        exit(0);
    }

    SDL_Surface* surface = IMG_Load(image_path.c_str());
    if (surface == nullptr) {
        std::cout << "error: missing image " << image_name;
        exit(0);
    }

    ui_system::gl_texture_info info;
    info.texture_id = create_surface_texture(surface, false);
    info.w = static_cast<float>(surface->w);
    info.h = static_cast<float>(surface->h);
    SDL_FreeSurface(surface);

    if (info.texture_id == 0) {
        std::cout << "error: failed to upload image " << image_name;
        exit(0);
    }

    auto insert_result = ui.gl_image_cache.emplace(image_name, std::move(info));
    return &insert_result.first->second;
}


ui_system::gl_texture_info* renderer3d::cache_ui_text(const ui_system::text_draw_request& request) {
    ui_system& ui = engine::current_engine->my_ui_system;
    const std::string cache_key = ui.get_text_cache_key(request);
    auto it = ui.gl_text_cache.find(cache_key);
    if (it != ui.gl_text_cache.end())
        return &it->second;

    TTF_Font* font = ui.cache_font(request.font_name, request.font_size);
    SDL_Color color = { request.r, request.g, request.b, request.a };
    SDL_Surface* surface = TTF_RenderUTF8_Blended(font, request.content.c_str(), color);
    if (surface == nullptr) {
        std::cout << "error: failed to render text";
        exit(0);
    }

    ui_system::gl_texture_info info;
    info.texture_id = create_surface_texture(surface, false);
    info.w = static_cast<float>(surface->w);
    info.h = static_cast<float>(surface->h);
    SDL_FreeSurface(surface);

    if (info.texture_id == 0) {
        std::cout << "error: failed to upload text";
        exit(0);
    }

    auto insert_result = ui.gl_text_cache.emplace(cache_key, std::move(info));
    return &insert_result.first->second;
}

void renderer3d::set_clear_color(float r, float g, float b, float a) {
    clear_color = glm::vec4(r, g, b, a);
}

void renderer3d::render_ui(ui_system& ui) {
    if (ui.image_requests.empty() && ui.text_requests.empty() && ui.pixel_requests.empty())
        return;

    int window_width = 1280;
    int window_height = 720;
    if (window != nullptr)
        SDL_GetWindowSize(window, &window_width, &window_height);

    const glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(window_width), static_cast<float>(window_height), 0.0f, -1.0f, 1.0f);
    std::vector<GpuUiVertex> ui_vertices;
    ui_vertices.reserve(4);

    glUseProgram(ui_shader_program);
    glBindVertexArray(ui_vertex_array_object);
    glActiveTexture(GL_TEXTURE0);
    glUniformMatrix4fv(glGetUniformLocation(ui_shader_program, "uProjection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniform1i(glGetUniformLocation(ui_shader_program, "uTexture"), 0);

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (ui.image_requests.size() > 1 && !std::is_sorted(ui.image_requests.begin(), ui.image_requests.end(), [](const ui_system::image_draw_request& left, const ui_system::image_draw_request& right) { return left.sorting_order < right.sorting_order; }))
        std::stable_sort(ui.image_requests.begin(), ui.image_requests.end(), [](const ui_system::image_draw_request& left, const ui_system::image_draw_request& right) { return left.sorting_order < right.sorting_order; });

    auto draw_ui_quad = [&](unsigned int texture_id, float x, float y, float width, float height, float pivot_x, float pivot_y, float rotation_degrees, const glm::vec4& color) {
        fill_ui_quad(ui_vertices, x, y, width, height, pivot_x, pivot_y, rotation_degrees, color);
        glBindTexture(GL_TEXTURE_2D, texture_id);
        glBindBuffer(GL_ARRAY_BUFFER, ui_vertex_buffer_object);
        glBufferData(GL_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(ui_vertices.size() * sizeof(GpuUiVertex)),
                     ui_vertices.data(),
                     GL_DYNAMIC_DRAW);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
    };

    for (const ui_system::image_draw_request& request : ui.image_requests) {
        ui_system::gl_texture_info* info = cache_ui_image(request.image_name);
        if (info == nullptr || info->texture_id == 0)
            continue;

        const glm::vec4 color(
            static_cast<float>(request.r) / 255.0f,
            static_cast<float>(request.g) / 255.0f,
            static_cast<float>(request.b) / 255.0f,
            static_cast<float>(request.a) / 255.0f
        );
        draw_ui_quad(info->texture_id,
                     request.x,
                     request.y,
                     info->w * request.scale_x,
                     info->h * request.scale_y,
                     request.pivot_x,
                     request.pivot_y,
                     request.rotation_degrees,
                     color);
    }
    for (const ui_system::text_draw_request& request : ui.text_requests) {
        ui_system::gl_texture_info* info = cache_ui_text(request);
        if (info == nullptr || info->texture_id == 0)
            continue;
        draw_ui_quad(info->texture_id,
                     static_cast<float>(request.x),
                     static_cast<float>(request.y),
                     info->w,
                     info->h,
                     0.0f,
                     0.0f,
                     0.0f,
                     glm::vec4(1.0f));
    }
    for (const ui_system::pixel_draw_request& request : ui.pixel_requests) {
        const glm::vec4 color(
            static_cast<float>(request.r) / 255.0f,
            static_cast<float>(request.g) / 255.0f,
            static_cast<float>(request.b) / 255.0f,
            static_cast<float>(request.a) / 255.0f
        );
        draw_ui_quad(ui_white_texture,
                     static_cast<float>(request.x),
                     static_cast<float>(request.y),
                     1.0f,
                     1.0f,
                     0.0f,
                     0.0f,
                     0.0f,
                     color);
    }
    glDisable(GL_BLEND);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindVertexArray(0);
    glUseProgram(0);
    glEnable(GL_DEPTH_TEST);
    ui.clear();
}

void renderer3d::render() {
    if (current_renderer != this)
        current_renderer = this;

    if (window == nullptr || gl_context == nullptr || shader_program == 0 || ui_shader_program == 0)
        return;

    SDL_GL_MakeCurrent(window, gl_context);

    int window_width = 1280;
    int window_height = 720;
    if (window != nullptr)
        SDL_GetWindowSize(window, &window_width, &window_height);
    glViewport(0, 0, window_width, window_height);

    glClearColor(clear_color.r, clear_color.g, clear_color.b, clear_color.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (!mesh_requests.empty()) {
        glEnable(GL_DEPTH_TEST);
        glUseProgram(shader_program);
        glBindVertexArray(vertex_array_object);

        const GLint model_location = glGetUniformLocation(shader_program, "uModel");
        const GLint view_location = glGetUniformLocation(shader_program, "uView");
        const GLint projection_location = glGetUniformLocation(shader_program, "uProjection");
        const GLint texture_location = glGetUniformLocation(shader_program, "uTexture");

        std::vector<GpuVertex3D> gpu_vertices;
        std::vector<std::uint32_t> gpu_indices;

        glActiveTexture(GL_TEXTURE0);
        glUniform1i(texture_location, 0);

        for (const mesh_draw_request& request : mesh_requests) {
            build_gpu_buffers(request, gpu_vertices, gpu_indices);
            if (gpu_vertices.empty() || gpu_indices.empty())
                continue;

            const glm::mat4 model = request.transform.GetModelMatrix();
            const glm::mat4 view = request.camera.GetViewMatrix();
            const glm::mat4 projection = request.camera.GetProjectionMatrix();

            glUniformMatrix4fv(model_location, 1, GL_FALSE, glm::value_ptr(model));
            glUniformMatrix4fv(view_location, 1, GL_FALSE, glm::value_ptr(view));
            glUniformMatrix4fv(projection_location, 1, GL_FALSE, glm::value_ptr(projection));

            material_texture_info* texture = cache_material_texture(request.material);
            glBindTexture(GL_TEXTURE_2D, texture == nullptr ? ui_white_texture : texture->texture_id);

            glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_object);
            glBufferData(GL_ARRAY_BUFFER,
                         static_cast<GLsizeiptr>(gpu_vertices.size() * sizeof(GpuVertex3D)),
                         gpu_vertices.data(),
                         GL_DYNAMIC_DRAW);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, element_buffer_object);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                         static_cast<GLsizeiptr>(gpu_indices.size() * sizeof(std::uint32_t)),
                         gpu_indices.data(),
                         GL_DYNAMIC_DRAW);

            glDrawElements(GL_TRIANGLES,
                           static_cast<GLsizei>(gpu_indices.size()),
                           GL_UNSIGNED_INT,
                           nullptr);
        }

        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glUseProgram(0);
    }

    render_ui(engine::current_engine->my_ui_system);

    SDL_GL_SwapWindow(window);
    clear_queue();
}

void renderer3d::end() {
    clear_queue();

    ui_system& ui = engine::current_engine->my_ui_system;

    for (auto& entry : material_texture_cache) {
        if (entry.second.texture_id != 0)
            glDeleteTextures(1, reinterpret_cast<GLuint*>(&entry.second.texture_id));
    }
    material_texture_cache.clear();

    for (auto& entry : ui.gl_image_cache) {
        if (entry.second.texture_id != 0)
            glDeleteTextures(1, reinterpret_cast<GLuint*>(&entry.second.texture_id));
    }
    ui.gl_image_cache.clear();

    for (auto& entry : ui.gl_text_cache) {
        if (entry.second.texture_id != 0)
            glDeleteTextures(1, reinterpret_cast<GLuint*>(&entry.second.texture_id));
    }
    ui.gl_text_cache.clear();

    if (ui_white_texture != 0) {
        glDeleteTextures(1, reinterpret_cast<GLuint*>(&ui_white_texture));
        ui_white_texture = 0;
    }
    if (ui_vertex_buffer_object != 0) {
        glDeleteBuffers(1, reinterpret_cast<GLuint*>(&ui_vertex_buffer_object));
        ui_vertex_buffer_object = 0;
    }
    if (ui_element_buffer_object != 0) {
        glDeleteBuffers(1, reinterpret_cast<GLuint*>(&ui_element_buffer_object));
        ui_element_buffer_object = 0;
    }
    if (ui_vertex_array_object != 0) {
        glDeleteVertexArrays(1, reinterpret_cast<GLuint*>(&ui_vertex_array_object));
        ui_vertex_array_object = 0;
    }
    if (ui_shader_program != 0) {
        glDeleteProgram(ui_shader_program);
        ui_shader_program = 0;
    }

    if (vertex_buffer_object != 0) {
        glDeleteBuffers(1, reinterpret_cast<GLuint*>(&vertex_buffer_object));
        vertex_buffer_object = 0;
    }
    if (element_buffer_object != 0) {
        glDeleteBuffers(1, reinterpret_cast<GLuint*>(&element_buffer_object));
        element_buffer_object = 0;
    }
    if (vertex_array_object != 0) {
        glDeleteVertexArrays(1, reinterpret_cast<GLuint*>(&vertex_array_object));
        vertex_array_object = 0;
    }
    if (shader_program != 0) {
        glDeleteProgram(shader_program);
        shader_program = 0;
    }
    if (gl_context != nullptr) {
        SDL_GL_DeleteContext(gl_context);
        gl_context = nullptr;
    }
    if (window != nullptr) {
        SDL_DestroyWindow(window);
        window = nullptr;
    }

    current_renderer = nullptr;
}

void renderer3d::clear_queue() {
    mesh_requests.clear();
}
