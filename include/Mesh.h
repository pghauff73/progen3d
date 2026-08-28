#pragma once

#include <glm/glm.hpp>

#include <array>
#include <string>
#include <vector>

#include "ProGen3dGl.h"

class MeshGeometryData
{
public:
    virtual ~MeshGeometryData() = default;

    bool hasFaces() const
    {
        return !faces.empty();
    }

    std::vector<glm::vec3> vertices;
    std::vector<glm::ivec3> faces;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec3> face_normals;
    std::vector<glm::vec3> texcoords;
};

class MeshCollisionAcceleration
{
public:
    struct CollisionBounds {
        glm::vec3 min{0.0f};
        glm::vec3 max{0.0f};
        glm::vec3 center{0.0f};
        glm::vec3 half_extents{0.0f};
        bool valid = false;
    };

    struct CollisionBvhNode {
        CollisionBounds bounds;
        int left = -1;
        int right = -1;
        int start = 0;
        int count = 0;

        bool isLeaf() const
        {
            return left < 0 && right < 0;
        }
    };

    virtual ~MeshCollisionAcceleration() = default;

    void clearCollisionAcceleration()
    {
        triangle_bounds.clear();
        collision_bvh_nodes.clear();
        collision_bvh_triangle_indices.clear();
    }

    bool hasCollisionAcceleration() const
    {
        return !collision_bvh_nodes.empty();
    }

    std::vector<CollisionBounds> triangle_bounds;
    std::vector<CollisionBvhNode> collision_bvh_nodes;
    std::vector<int> collision_bvh_triangle_indices;
};

class Mesh : public MeshGeometryData, public MeshCollisionAcceleration
{
public:
    using CollisionBounds = MeshCollisionAcceleration::CollisionBounds;
    using CollisionBvhNode = MeshCollisionAcceleration::CollisionBvhNode;

    Mesh();
    ~Mesh();

    void add(const Mesh &other);
    void apply(const glm::mat4 &transform);
    void addTriangle(int i0, int i1, int i2);
    void addQuad(int i0, int i1, int i2, int i3);
    static Mesh getInstance(const std::string &objId);
    static const Mesh &getSharedInstance(const std::string &objId);
    static bool isStlInstanceType(const std::string &objId);
    static std::string stlFileStemForInstanceType(const std::string &objId);
    void draw();
    void calc_normals();
    void buildCollisionAccel();
    const std::vector<CollisionBounds> &getTriangleBounds() const;
    const std::vector<CollisionBvhNode> &getCollisionBvhNodes() const;
    const std::vector<int> &getCollisionBvhTriangleIndices() const;

private:
    static Mesh getCube();
    static Mesh getSphere();
    static Mesh getCylinder();
};
