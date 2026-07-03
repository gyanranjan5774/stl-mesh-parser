#include "mesh/Mesh.hpp"

#include <cstring>
#include <stdexcept>

namespace mesh {

namespace {
std::uint32_t floatBits(double d) {
    float f = static_cast<float>(d);
    if (f == 0.0f) f = 0.0f;  // normalise -0.0f to +0.0f
    std::uint32_t u;
    std::memcpy(&u, &f, sizeof u);
    return u;
}
}  // namespace

std::size_t Mesh::KeyHash::operator()(const Key& k) const {
    // FNV-1a style mixing over the three coordinate bit patterns.
    std::uint64_t h = 1469598103934665603ull;
    for (std::uint32_t b : k.bits) {
        h ^= b;
        h *= 1099511628211ull;
    }
    return static_cast<std::size_t>(h);
}

std::uint32_t Mesh::addVertex(const Vec3& p) {
    const Key key{{floatBits(p.x), floatBits(p.y), floatBits(p.z)}};
    auto it = weld_.find(key);
    if (it != weld_.end()) return it->second;
    const auto index = static_cast<std::uint32_t>(vertices_.size());
    vertices_.push_back(p);
    weld_.emplace(key, index);
    return index;
}

void Mesh::addTriangle(const Vec3& a, const Vec3& b, const Vec3& c) {
    const auto ia = addVertex(a);
    const auto ib = addVertex(b);
    const auto ic = addVertex(c);
    triangles_.push_back({ia, ib, ic});
}

void Mesh::addTriangleByIndex(std::uint32_t a, std::uint32_t b, std::uint32_t c) {
    const auto n = vertices_.size();
    if (a >= n || b >= n || c >= n) throw std::out_of_range("Mesh::addTriangleByIndex: bad vertex index");
    triangles_.push_back({a, b, c});
}

void Mesh::reserve(std::size_t triangleCount) {
    triangles_.reserve(triangleCount);
    vertices_.reserve(triangleCount / 2 + 8);  // closed mesh: V ~ F/2
    weld_.reserve(triangleCount / 2 + 8);
}

std::array<Vec3, 3> Mesh::corners(std::size_t i) const {
    const Triangle& t = triangles_.at(i);
    return {vertices_[t[0]], vertices_[t[1]], vertices_[t[2]]};
}

}  // namespace mesh
