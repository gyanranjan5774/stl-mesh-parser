#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>

#include "mesh/Vec3.hpp"

namespace mesh {

using Triangle = std::array<std::uint32_t, 3>;

// Indexed triangle mesh. STL stores every triangle with its own 3 vertices
// ("triangle soup"), so the same corner is repeated by every face that touches it.
// addTriangle() *welds* identical vertices through a hash map (O(1) average per
// vertex), which gives the shared-vertex connectivity that topology checks need.
class Mesh {
public:
    // Returns the index of p, inserting it if it has not been seen before.
    // Two points weld if their float32 representations are bit-identical
    // (-0.0 and +0.0 are treated as equal).
    std::uint32_t addVertex(const Vec3& p);

    void addTriangle(const Vec3& a, const Vec3& b, const Vec3& c);
    void addTriangleByIndex(std::uint32_t a, std::uint32_t b, std::uint32_t c);

    void reserve(std::size_t triangleCount);

    const std::vector<Vec3>& vertices() const { return vertices_; }
    const std::vector<Triangle>& triangles() const { return triangles_; }
    std::size_t vertexCount() const { return vertices_.size(); }
    std::size_t triangleCount() const { return triangles_.size(); }

    // Convenience: the three corner positions of triangle i.
    std::array<Vec3, 3> corners(std::size_t i) const;

private:
    struct Key {
        std::uint32_t bits[3];
        bool operator==(const Key& o) const {
            return bits[0] == o.bits[0] && bits[1] == o.bits[1] && bits[2] == o.bits[2];
        }
    };
    struct KeyHash {
        std::size_t operator()(const Key& k) const;
    };

    std::vector<Vec3> vertices_;
    std::vector<Triangle> triangles_;
    std::unordered_map<Key, std::uint32_t, KeyHash> weld_;
};

}  // namespace mesh
