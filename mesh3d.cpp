#include "mesh3d.h"
#include <fstream>
#include <sstream>
#include <unordered_map>

namespace {

int parse_obj_index(const std::string& token, int count) {
    if (token.empty())
        return -1;

    int index = std::stoi(token);
    if (index < 0)
        return count + index;
    return index - 1;
}

}

void Mesh3D::Clear() {
    name.clear();
    vertices.clear();
    indices.clear();
}

bool Mesh3D::LoadObj(const std::filesystem::path& path) {
    return LoadObj(path.string());
}

bool Mesh3D::LoadObj(const std::string& path_string) {
    Clear();
    std::ifstream file(path_string);
    if (!file.is_open())
        return false;

    std::vector<glm::vec3> positions;
    std::vector<glm::vec2> uvs;
    std::unordered_map<std::string, std::uint32_t> vertex_lookup;
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line.front() == '#')
            continue;

        std::istringstream stream(line);
        std::string token;
        stream >> token;
        if (token == "v") {
            float x = 0.0f;
            float y = 0.0f;
            float z = 0.0f;
            stream >> x >> y >> z;
            positions.emplace_back(x, y, z);
            continue;
        }
        if (token == "vt") {
            float u = 0.0f;
            float v = 0.0f;
            stream >> u >> v;
            uvs.emplace_back(u, v);
            continue;
        }
        if (token == "f") {
            std::vector<std::uint32_t> face;
            std::string vertex_token;
            while (stream >> vertex_token) {
                std::string position_token = vertex_token;
                std::string uv_token;

                const std::size_t first_slash = vertex_token.find('/');
                if (first_slash != std::string::npos) {
                    position_token = vertex_token.substr(0, first_slash);
                    const std::size_t second_slash = vertex_token.find('/', first_slash + 1);
                    if (second_slash == std::string::npos)
                        uv_token = vertex_token.substr(first_slash + 1);
                    else
                        uv_token = vertex_token.substr(first_slash + 1, second_slash - first_slash - 1);
                }

                const int position_index = parse_obj_index(position_token, static_cast<int>(positions.size()));
                if (position_index < 0 || position_index >= static_cast<int>(positions.size()))
                    continue;

                const int uv_index = parse_obj_index(uv_token, static_cast<int>(uvs.size()));
                const std::string key = std::to_string(position_index) + "/" + std::to_string(uv_index);
                auto lookup = vertex_lookup.find(key);
                if (lookup != vertex_lookup.end()) {
                    face.emplace_back(lookup->second);
                    continue;
                }

                Vertex3D vertex;
                vertex.position = positions[static_cast<std::size_t>(position_index)];
                if (uv_index >= 0 && uv_index < static_cast<int>(uvs.size()))
                    vertex.uv = uvs[static_cast<std::size_t>(uv_index)];
                vertices.emplace_back(vertex);
                const std::uint32_t new_index = static_cast<std::uint32_t>(vertices.size() - 1);
                vertex_lookup.emplace(key, new_index);
                face.emplace_back(new_index);
            }
            if (face.size() < 3)
                continue;
            for (std::size_t i = 1; i + 1 < face.size(); ++i) {
                indices.emplace_back(face[0]);
                indices.emplace_back(face[i]);
                indices.emplace_back(face[i + 1]);
            }
        }
    }

    return !vertices.empty() && !indices.empty();
}

bool Mesh3D::HasVertices() const {
    return !vertices.empty();
}

bool Mesh3D::HasIndices() const {
    return !indices.empty();
}

std::size_t Mesh3D::GetTriangleCount() const {
    return indices.size() / 3;
}
