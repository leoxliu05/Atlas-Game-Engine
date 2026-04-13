#ifndef MESH3D_H
#define MESH3D_H

#include "glm.hpp"
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

struct Vertex3D {
    glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 normal = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec2 uv = glm::vec2(0.0f, 0.0f);
};

struct Mesh3D {
    std::string name = "";
    std::vector<Vertex3D> vertices;
    std::vector<std::uint32_t> indices;

    void Clear(); // api 3d
    bool LoadObj(const std::string& path); // api 3d
    bool LoadObj(const std::filesystem::path& path);
    bool HasVertices() const; // api 3d
    bool HasIndices() const; // api 3d
    std::size_t GetTriangleCount() const; // api 3d
};

#endif // MESH3D_H
