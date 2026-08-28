#include "Mesh.h"

#include "AppPaths.h"

#include "glm/ext.hpp"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <mutex>
#include <numeric>
#include <sstream>
#include <unordered_map>

namespace {

bool starts_with(const std::string &value, const std::string &prefix)
{
    return value.size() >= prefix.size() &&
           value.compare(0, prefix.size(), prefix) == 0;
}

std::filesystem::path resolve_stl_path(const std::string &objId)
{
    const std::string file_stem = Mesh::stlFileStemForInstanceType(objId);
    return progen3d_resource_path(std::filesystem::path("stls") / (file_stem + ".stl"));
}

void clear_collision_accel(Mesh *mesh)
{
    if (mesh == nullptr) {
        return;
    }
    mesh->clearCollisionAcceleration();
}

Mesh::CollisionBounds merge_bounds(const Mesh::CollisionBounds &a, const Mesh::CollisionBounds &b)
{
    if (!a.valid) {
        return b;
    }
    if (!b.valid) {
        return a;
    }

    Mesh::CollisionBounds bounds;
    bounds.min = glm::min(a.min, b.min);
    bounds.max = glm::max(a.max, b.max);
    bounds.center = (bounds.min + bounds.max) * 0.5f;
    bounds.half_extents = (bounds.max - bounds.min) * 0.5f;
    bounds.valid = true;
    return bounds;
}

Mesh::CollisionBounds bounds_for_triangle(const Mesh &mesh, int triangle_index)
{
    Mesh::CollisionBounds bounds;
    if (triangle_index < 0 || static_cast<std::size_t>(triangle_index) >= mesh.faces.size()) {
        return bounds;
    }

    const glm::ivec3 &face = mesh.faces[static_cast<std::size_t>(triangle_index)];
    bounds.min = glm::vec3(std::numeric_limits<float>::max());
    bounds.max = glm::vec3(std::numeric_limits<float>::lowest());
    const glm::vec3 vertices[3] = {
        mesh.vertices[static_cast<std::size_t>(face.x)],
        mesh.vertices[static_cast<std::size_t>(face.y)],
        mesh.vertices[static_cast<std::size_t>(face.z)]};
    for (const glm::vec3 &vertex : vertices) {
        bounds.min = glm::min(bounds.min, vertex);
        bounds.max = glm::max(bounds.max, vertex);
    }
    bounds.center = (bounds.min + bounds.max) * 0.5f;
    bounds.half_extents = (bounds.max - bounds.min) * 0.5f;
    bounds.valid = true;
    return bounds;
}

Mesh::CollisionBounds bounds_for_triangle_range(const Mesh &mesh,
                                                const std::vector<int> &triangle_indices,
                                                int start,
                                                int count)
{
    Mesh::CollisionBounds bounds;
    for (int offset = 0; offset < count; ++offset) {
        const int triangle_index = triangle_indices[static_cast<std::size_t>(start + offset)];
        if (triangle_index < 0 || static_cast<std::size_t>(triangle_index) >= mesh.triangle_bounds.size()) {
            continue;
        }
        bounds = merge_bounds(bounds, mesh.triangle_bounds[static_cast<std::size_t>(triangle_index)]);
    }
    return bounds;
}

int longest_axis(const Mesh::CollisionBounds &bounds)
{
    const glm::vec3 full_extents = bounds.half_extents * 2.0f;
    if (full_extents.y > full_extents.x && full_extents.y >= full_extents.z) {
        return 1;
    }
    if (full_extents.z > full_extents.x && full_extents.z > full_extents.y) {
        return 2;
    }
    return 0;
}

int build_collision_bvh_node(const Mesh &mesh,
                             std::vector<int> *triangle_indices,
                             int start,
                             int count,
                             std::vector<Mesh::CollisionBvhNode> *nodes)
{
    if (triangle_indices == nullptr || nodes == nullptr || count <= 0) {
        return -1;
    }

    Mesh::CollisionBvhNode node;
    node.bounds = bounds_for_triangle_range(mesh, *triangle_indices, start, count);
    node.start = start;
    node.count = count;
    const int node_index = static_cast<int>(nodes->size());
    nodes->push_back(node);

    if (count <= 4) {
        return node_index;
    }

    const int axis = longest_axis(node.bounds);
    const int mid = start + (count / 2);
    std::nth_element(triangle_indices->begin() + start,
                     triangle_indices->begin() + mid,
                     triangle_indices->begin() + start + count,
                     [&mesh, axis](int a, int b) {
                         return mesh.triangle_bounds[static_cast<std::size_t>(a)].center[axis] <
                                mesh.triangle_bounds[static_cast<std::size_t>(b)].center[axis];
                     });

    const int left = build_collision_bvh_node(mesh, triangle_indices, start, mid - start, nodes);
    const int right = build_collision_bvh_node(mesh, triangle_indices, mid, count - (mid - start), nodes);
    (*nodes)[static_cast<std::size_t>(node_index)].left = left;
    (*nodes)[static_cast<std::size_t>(node_index)].right = right;
    (*nodes)[static_cast<std::size_t>(node_index)].start = 0;
    (*nodes)[static_cast<std::size_t>(node_index)].count = 0;
    return node_index;
}

void add_stl_triangle(Mesh *mesh,
                      const glm::vec3 &normal,
                      const glm::vec3 &a,
                      const glm::vec3 &b,
                      const glm::vec3 &c)
{
    if (mesh == nullptr) {
        return;
    }

    const int base_index = static_cast<int>(mesh->vertices.size());
    mesh->vertices.push_back(a);
    mesh->vertices.push_back(b);
    mesh->vertices.push_back(c);
    mesh->faces.emplace_back(base_index, base_index + 1, base_index + 2);
    mesh->face_normals.push_back(normal);
}

bool load_ascii_stl(std::istream &input, Mesh *mesh)
{
    if (mesh == nullptr) {
        return false;
    }

    std::string token;
    glm::vec3 current_normal(0.0f, 1.0f, 0.0f);
    std::vector<glm::vec3> triangle_vertices;
    triangle_vertices.reserve(3);

    while (input >> token) {
        if (token == "facet") {
            std::string normal_token;
            float nx = 0.0f;
            float ny = 0.0f;
            float nz = 0.0f;
            if (!(input >> normal_token >> nx >> ny >> nz)) {
                return false;
            }
            current_normal = glm::vec3(nx, ny, nz);
            continue;
        }

        if (token == "vertex") {
            float x = 0.0f;
            float y = 0.0f;
            float z = 0.0f;
            if (!(input >> x >> y >> z)) {
                return false;
            }
            triangle_vertices.emplace_back(x, y, z);
            if (triangle_vertices.size() == 3) {
                add_stl_triangle(mesh,
                                 current_normal,
                                 triangle_vertices[0],
                                 triangle_vertices[1],
                                 triangle_vertices[2]);
                triangle_vertices.clear();
            }
        }
    }

    return !mesh->faces.empty();
}

bool load_binary_stl(std::istream &input, std::uint32_t triangle_count, Mesh *mesh)
{
    if (mesh == nullptr) {
        return false;
    }

    for (std::uint32_t triangle_index = 0; triangle_index < triangle_count; ++triangle_index) {
        float normal_components[3];
        float vertex_components[9];
        std::uint16_t attribute_byte_count = 0;
        if (!input.read(reinterpret_cast<char *>(normal_components), sizeof(normal_components)) ||
            !input.read(reinterpret_cast<char *>(vertex_components), sizeof(vertex_components)) ||
            !input.read(reinterpret_cast<char *>(&attribute_byte_count), sizeof(attribute_byte_count))) {
            return false;
        }

        add_stl_triangle(mesh,
                         glm::vec3(normal_components[0], normal_components[1], normal_components[2]),
                         glm::vec3(vertex_components[0], vertex_components[1], vertex_components[2]),
                         glm::vec3(vertex_components[3], vertex_components[4], vertex_components[5]),
                         glm::vec3(vertex_components[6], vertex_components[7], vertex_components[8]));
    }

    return true;
}

Mesh load_stl_mesh(const std::string &objId)
{
    Mesh mesh;
    const std::filesystem::path stl_path = resolve_stl_path(objId);
    std::error_code error;
    if (!std::filesystem::exists(stl_path, error) || !std::filesystem::is_regular_file(stl_path, error)) {
        std::cerr << "W: STL instance file not found: " << stl_path.string() << std::endl;
        return mesh;
    }

    const auto file_size = std::filesystem::file_size(stl_path, error);
    if (error) {
        std::cerr << "W: Could not read STL file size: " << stl_path.string() << std::endl;
        return mesh;
    }

    std::ifstream input(stl_path, std::ios::binary);
    if (!input.is_open()) {
        std::cerr << "W: Could not open STL instance file: " << stl_path.string() << std::endl;
        return mesh;
    }

    char header[80] = {};
    std::uint32_t triangle_count = 0;
    if (file_size >= 84 &&
        input.read(header, sizeof(header)) &&
        input.read(reinterpret_cast<char *>(&triangle_count), sizeof(triangle_count))) {
        const std::uintmax_t expected_size = 84u + (static_cast<std::uintmax_t>(triangle_count) * 50u);
        if (expected_size == file_size) {
            input.clear();
            input.seekg(84, std::ios::beg);
            if (load_binary_stl(input, triangle_count, &mesh)) {
                mesh.buildCollisionAccel();
                return mesh;
            }
            mesh = Mesh();
        }
    }

    input.clear();
    input.seekg(0, std::ios::beg);
    if (!load_ascii_stl(input, &mesh)) {
        std::cerr << "W: Failed to parse STL instance file: " << stl_path.string() << std::endl;
        return Mesh();
    }

    mesh.buildCollisionAccel();
    return mesh;
}

std::unordered_map<std::string, Mesh> &stl_mesh_cache()
{
    static std::unordered_map<std::string, Mesh> cache;
    return cache;
}

std::mutex &stl_mesh_cache_mutex()
{
    static std::mutex mutex;
    return mutex;
}

} // namespace

Mesh::Mesh() = default;

Mesh::~Mesh()
{

}
void Mesh::draw(){
	
	
}

void Mesh::calc_normals(){
	
	
	
	normals.clear();
	face_normals.clear();
	
	for(int i=0;i<faces.size();i++){
		
		glm::ivec3 face=faces[i];
		
		
		glm::vec3 A=vertices[face[1]]-vertices[face[0]];
		glm::vec3 B=vertices[face[2]]-vertices[face[0]];
		
		glm::vec3 normal=glm::cross(A,B);
		normals.push_back(normal);
        face_normals.push_back(normal);
		
	}
	
	
}

void Mesh::apply(const glm::mat4 &transform)
{
    clear_collision_accel(this);
    for (auto &vertex : vertices)
        vertex = glm::vec3(transform * glm::vec4(vertex, 1.0));
}

void Mesh::add(const Mesh &other)
{
    clear_collision_accel(this);
    int numVertices = vertices.size();
    vertices.insert(vertices.end(), other.vertices.begin(), other.vertices.end());
    face_normals.insert(face_normals.end(), other.face_normals.begin(), other.face_normals.end());

    for (const auto &face : other.faces)
        faces.emplace_back(face + glm::ivec3(numVertices));
}

void Mesh::addTriangle(int i0, int i1, int i2)
{
    clear_collision_accel(this);
    faces.emplace_back(i0, i1, i2);
}

void Mesh::addQuad(int i0, int i1, int i2, int i3)
{
    addTriangle(i0, i1, i2);
    addTriangle(i0, i2, i3);
}

void Mesh::buildCollisionAccel()
{
    triangle_bounds.clear();
    collision_bvh_nodes.clear();
    collision_bvh_triangle_indices.clear();
    if (faces.empty()) {
        return;
    }

    triangle_bounds.reserve(faces.size());
    for (std::size_t face_index = 0; face_index < faces.size(); ++face_index) {
        triangle_bounds.push_back(bounds_for_triangle(*this, static_cast<int>(face_index)));
    }

    collision_bvh_triangle_indices.resize(faces.size());
    std::iota(collision_bvh_triangle_indices.begin(), collision_bvh_triangle_indices.end(), 0);
    collision_bvh_nodes.reserve(faces.size() * 2u);
    build_collision_bvh_node(*this,
                             &collision_bvh_triangle_indices,
                             0,
                             static_cast<int>(collision_bvh_triangle_indices.size()),
                             &collision_bvh_nodes);
}

const std::vector<Mesh::CollisionBounds> &Mesh::getTriangleBounds() const
{
    return triangle_bounds;
}

const std::vector<Mesh::CollisionBvhNode> &Mesh::getCollisionBvhNodes() const
{
    return collision_bvh_nodes;
}

const std::vector<int> &Mesh::getCollisionBvhTriangleIndices() const
{
    return collision_bvh_triangle_indices;
}

Mesh Mesh::getInstance(const std::string &objId)
{
    return getSharedInstance(objId);
}

const Mesh &Mesh::getSharedInstance(const std::string &objId)
{
    static const Mesh cube = getCube();
    static const Mesh cylinder = getCylinder();
    static const Mesh sphere = getSphere();
    static const Mesh empty;

    if (objId == "Cylinder") {
        return cylinder;
    }

    if (objId == "Sphere") {
        return sphere;
    }

    if (isStlInstanceType(objId)) {
        std::lock_guard<std::mutex> lock(stl_mesh_cache_mutex());
        auto &cache = stl_mesh_cache();
        auto existing = cache.find(objId);
        if (existing != cache.end()) {
            return existing->second;
        }
        return cache.emplace(objId, load_stl_mesh(objId)).first->second;
    }

    if (objId != "Cube" && objId != "CubeX" && objId != "CubeY" && objId != "CubeZ") {
        return empty;
    }

    return cube;
}

bool Mesh::isStlInstanceType(const std::string &objId)
{
    (void)objId;
    return false;
}

std::string Mesh::stlFileStemForInstanceType(const std::string &objId)
{
    if (!isStlInstanceType(objId)) {
        return "";
    }

    const std::string catalog_id = objId.substr(4);
    if (catalog_id.empty()) {
        return "";
    }

    const std::filesystem::path stl_root = progen3d_resource_path("stls");
    const std::filesystem::path legacy_path = (stl_root / (catalog_id + ".stl")).lexically_normal();
    std::error_code error;
    if (std::filesystem::exists(legacy_path, error) && !error) {
        return catalog_id;
    }

    const std::size_t category_separator = catalog_id.rfind('.');
    if (category_separator != std::string::npos && category_separator + 1 < catalog_id.size()) {
        return catalog_id.substr(category_separator + 1);
    }

    return catalog_id;
}

Mesh Mesh::getCube()
{
    Mesh cube;

    cube.vertices = {{0.5, 0, 0.5}, {0.5, 1, 0.5}, {-0.5, 0, 0.5}, {-0.5, 1, 0.5},
                    {0.5, 0, -0.5}, {0.5, 1, -0.5}, {-0.5, 0, -0.5}, {-0.5, 1, -0.5}};

    cube.faces =    {{0, 3, 2}, {0, 1, 3}, // TOP
                    {5, 4, 7}, {4, 6, 7}, // BOTTOM
                    {4, 0, 6}, {0, 2, 6}, // LEFT
                    {5, 3, 1}, {5, 7, 3}, // RIGHT
                    {4, 1, 0}, {5, 1, 4}, // FRONT
                    {2, 3, 6}, {6, 3, 7}  // BACK
    };
    cube.texcoords = {{0.5, 0, 0.5}, {0.5, 1, 0.5}, {-0.5, 0, 0.5}, {-0.5, 1, 0.5},
                    {0.5, 0, -0.5}, {0.5, 1, -0.5}, {-0.5, 0, -0.5}, {-0.5, 1, -0.5}};

    return cube;
}

Mesh Mesh::getCylinder()
{
    const int n = 40;
    float R = 0.5f;
    float pi = glm::pi<float>();
    glm::vec3 translation(0.0f, 0.0f, 0.0f); // Translation that has to be added to make the origin be at the bottom left corner
    Mesh cylinder;

    // Bottom vertices
    cylinder.vertices.push_back(glm::vec3(0.0f, 0.0f, 0.0f) + translation);
    cylinder.texcoords.push_back(glm::vec3(0.0f, 0.0f, 0.0f));
    
    for (int i = 1; i < n; ++i) {
        float t = float(i)/float(n);
        glm::vec3 baseVertex = glm::vec3(R*glm::cos(2*pi*t), 0.0f, R*glm::sin(2*pi*t)) + translation;
        cylinder.vertices.push_back(baseVertex);
        cylinder.texcoords.push_back(glm::vec3(glm::cos(2*pi*t), 0.0f, glm::sin(2*pi*t)));
        cylinder.faces.push_back(glm::ivec3(i + 1, 0, i));
    }
    cylinder.vertices.push_back(glm::vec3(R, 0.0f, 0.0f) + translation);
    cylinder.texcoords.push_back(glm::vec3(1.0f, 0.0f, 0.0f) + translation);
    cylinder.faces.push_back(glm::ivec3(1, 0, n));

    int topOffset = n + 1;

    // Top Vertices
    cylinder.vertices.push_back(glm::vec3(0.0f, 1.0f, 0.0f) + translation);
    cylinder.texcoords.push_back(glm::vec3(0.0f, 1.0f, 0.0f) + translation);
    for (int i = 1; i < n; ++i) {
        float t = float(i)/float(n);
        glm::vec3 topVertex = glm::vec3(R*glm::cos(2*pi*t), 1.0f, R*glm::sin(2*pi*t)) + translation;
        cylinder.vertices.push_back(topVertex);
        cylinder.texcoords.push_back(glm::vec3(glm::cos(2*pi*t), 1.0f, glm::sin(2*pi*t)));
        cylinder.faces.push_back(glm::ivec3(i + topOffset, topOffset, i + 1 + topOffset));
        cylinder.faces.push_back(glm::ivec3(i + 1 + topOffset, i, i + topOffset));
        cylinder.faces.push_back(glm::ivec3(i, i + 1 + topOffset, i + 1));
    }
    cylinder.vertices.push_back(glm::vec3(R, 1.0f, 0.0f) + translation);
    cylinder.texcoords.push_back(glm::vec3(1.0f, 1.0f, 0.0f) + translation);
    cylinder.faces.push_back(glm::ivec3(n + topOffset, topOffset, 1 + topOffset));
    cylinder.faces.push_back(glm::ivec3(1 + topOffset, n, n + topOffset));
    cylinder.faces.push_back(glm::ivec3(n, 1 + topOffset, 1));

    return cylinder;
}


Mesh Mesh::getSphere()
{
    const int n_phi = 20; // Goes from 0 to pi
    const int n_theta = 40; // Goes from 0 to 2*pi
    const float R = 0.5f;
    const float pi = glm::pi<float>();
    Mesh sphere;

    // Add north pole
    glm::vec3 northPole = glm::vec3(0.0f, R, 0.0f);
    sphere.vertices.push_back(northPole);
	sphere.texcoords.push_back(glm::vec3(0.0f, 1.0f, 0.0f));
    // Add layer connected to north pole
    float t_phi = float(1)/float(n_phi);
    float c_phi = glm::cos(pi * t_phi);
    float s_phi = glm::sin(pi * t_phi);

    for (int i = 1; i < n_theta; ++i) {
        float t_theta = float(i)/float(n_theta);
        float c_theta = glm::cos(2*pi * t_theta);
        float s_theta = glm::sin(2*pi * t_theta);
		
        glm::vec3 vertex = R * glm::vec3(s_phi * c_theta, c_phi, s_phi * s_theta);
        sphere.texcoords.push_back(glm::vec3(s_phi * c_theta, c_phi, s_phi * s_theta));
        sphere.vertices.push_back(vertex);
        sphere.addTriangle(i+1, i, 0);
    }
    sphere.vertices.push_back(R * glm::vec3(s_phi, c_phi, 0.0f));
    sphere.texcoords.push_back( glm::vec3(s_phi, c_phi, 0.0f));
    sphere.addTriangle(1, n_theta, 0);

    int offset = n_theta;

    // Add middle layers
    for (int j = 2; j < n_phi; ++j) {
        t_phi = float(j)/float(n_phi);
        c_phi = glm::cos(pi * t_phi);
        s_phi = glm::sin(pi * t_phi);
        for (int i = 1; i < n_theta; ++i) {
            float t_theta = float(i)/float(n_theta);
            float c_theta = glm::cos(2*pi * t_theta);
            float s_theta = glm::sin(2*pi * t_theta);

            glm::vec3 vertex = R * glm::vec3(s_phi * c_theta, c_phi, s_phi * s_theta);
            sphere.vertices.push_back(vertex);
            sphere.texcoords.push_back(glm::vec3(s_phi * c_theta, c_phi, s_phi * s_theta));
            sphere.addQuad(i+1 + offset, i + offset, i + offset - n_theta, i+1 + offset - n_theta);
        }
        sphere.vertices.push_back(R * glm::vec3(s_phi, c_phi, 0.0f));
        sphere.texcoords.push_back(glm::vec3(s_phi, c_phi, 0.0f));
        sphere.addQuad(1 + offset, n_theta + offset, offset, 1 + offset - n_theta);
        offset += n_theta;
    }

    // Add layer connected to south pole
    for (int i = 1; i < n_theta; ++i) {
        float t_theta = float(i)/float(n_theta);
        float c_theta = glm::cos(2*pi * t_theta);
        float s_theta = glm::sin(2*pi * t_theta);
        glm::vec3 vertex = R * glm::vec3(s_phi * c_theta, c_phi, s_phi * s_theta);
        sphere.vertices.push_back(vertex);
        sphere.texcoords.push_back(glm::vec3(s_phi * c_theta, c_phi, s_phi * s_theta));
        sphere.addTriangle(i+1 + offset, n_theta + 1 + offset, i + offset);
    }
    sphere.vertices.push_back(R * glm::vec3(s_phi, c_phi, 0.0f));
    sphere.texcoords.push_back(glm::vec3(s_phi, c_phi, 0.0f));
    sphere.addTriangle(1 + offset, n_theta + 1 + offset, n_theta + offset);

    // Add north pole
    glm::vec3 southPole = glm::vec3(0.0f, -R, 0.0f);
    sphere.texcoords.push_back(glm::vec3(0.0f, -1.0f, 0.0f));
    sphere.vertices.push_back(southPole);
    
    return sphere;
}
