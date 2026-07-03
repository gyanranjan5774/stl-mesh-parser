#include "mesh/Analysis.hpp"

#include <cmath>
#include <cstdint>
#include <unordered_map>
#include <utility>
#include <vector>

namespace mesh {

namespace {

// Disjoint-set union with path halving + union by rank: near-O(1) amortised.
class DisjointSet {
public:
    explicit DisjointSet(std::size_t n) : parent_(n), rank_(n, 0) {
        for (std::size_t i = 0; i < n; ++i) parent_[i] = static_cast<std::uint32_t>(i);
    }
    std::uint32_t find(std::uint32_t x) {
        while (parent_[x] != x) {
            parent_[x] = parent_[parent_[x]];
            x = parent_[x];
        }
        return x;
    }
    void unite(std::uint32_t a, std::uint32_t b) {
        a = find(a);
        b = find(b);
        if (a == b) return;
        if (rank_[a] < rank_[b]) std::swap(a, b);
        parent_[b] = a;
        if (rank_[a] == rank_[b]) ++rank_[a];
    }

private:
    std::vector<std::uint32_t> parent_;
    std::vector<std::uint8_t> rank_;
};

std::uint64_t edgeKey(std::uint32_t lo, std::uint32_t hi) {
    return (static_cast<std::uint64_t>(lo) << 32) | hi;
}

}  // namespace

BoundingBox boundingBox(const Mesh& m) {
    BoundingBox bb;
    if (m.vertexCount() == 0) return bb;
    bb.min = bb.max = m.vertices().front();
    for (const Vec3& v : m.vertices()) {
        bb.min = {std::fmin(bb.min.x, v.x), std::fmin(bb.min.y, v.y), std::fmin(bb.min.z, v.z)};
        bb.max = {std::fmax(bb.max.x, v.x), std::fmax(bb.max.y, v.y), std::fmax(bb.max.z, v.z)};
    }
    return bb;
}

double surfaceArea(const Mesh& m) {
    double total = 0.0;
    const auto& V = m.vertices();
    for (const Triangle& t : m.triangles()) {
        total += 0.5 * length(cross(V[t[1]] - V[t[0]], V[t[2]] - V[t[0]]));
    }
    return total;
}

double signedVolume(const Mesh& m) {
    if (m.triangleCount() == 0) return 0.0;
    const Vec3 ref = boundingBox(m).center();
    const auto& V = m.vertices();
    double sum = 0.0;
    for (const Triangle& t : m.triangles()) {
        sum += dot(V[t[0]] - ref, cross(V[t[1]] - ref, V[t[2]] - ref));
    }
    return sum / 6.0;
}

Vec3 centroid(const Mesh& m) {
    if (m.triangleCount() == 0) return {};
    const Vec3 ref = boundingBox(m).center();
    const auto& V = m.vertices();
    double volume = 0.0;
    Vec3 weighted;
    for (const Triangle& t : m.triangles()) {
        const Vec3 a = V[t[0]] - ref, b = V[t[1]] - ref, c = V[t[2]] - ref;
        const double v = dot(a, cross(b, c)) / 6.0;  // signed tetra volume (apex = ref)
        volume += v;
        weighted = weighted + (a + b + c) * (v / 4.0);  // tetra centroid = (0 + a + b + c) / 4
    }
    if (std::fabs(volume) < 1e-300) return ref;  // flat / empty: no enclosed volume
    return ref + weighted / volume;
}

TopologyReport analyzeTopology(const Mesh& m) {
    TopologyReport r;
    const auto& V = m.vertices();
    const auto& T = m.triangles();
    r.faces = T.size();

    std::vector<char> used(V.size(), 0);
    DisjointSet dsu(V.size());
    // value = {count of lo->hi traversals, count of hi->lo traversals}
    std::unordered_map<std::uint64_t, std::pair<std::uint32_t, std::uint32_t>> edges;
    edges.reserve(T.size() * 3 / 2 + 16);

    for (const Triangle& t : T) {
        bool degenerate = t[0] == t[1] || t[1] == t[2] || t[0] == t[2];
        if (!degenerate) {
            degenerate = length(cross(V[t[1]] - V[t[0]], V[t[2]] - V[t[0]])) < 1e-12;
        }
        if (degenerate) ++r.degenerateTriangles;

        for (int i = 0; i < 3; ++i) {
            used[t[i]] = 1;
            dsu.unite(t[0], t[i]);
            const std::uint32_t u = t[i], v = t[(i + 1) % 3];
            if (u == v) continue;
            if (u < v) ++edges[edgeKey(u, v)].first;
            else ++edges[edgeKey(v, u)].second;
        }
    }

    for (const auto& kv : edges) {
        const std::uint32_t fwd = kv.second.first, bwd = kv.second.second;
        const std::uint32_t total = fwd + bwd;
        if (total == 1) ++r.boundaryEdges;
        else if (total > 2) ++r.nonManifoldEdges;
        else if (fwd != 1) ++r.inconsistentEdges;  // total == 2 but same direction twice
    }

    for (std::uint32_t i = 0; i < V.size(); ++i) {
        if (!used[i]) continue;
        ++r.vertices;
        if (dsu.find(i) == i) ++r.components;
    }
    r.edges = edges.size();
    r.eulerCharacteristic = static_cast<long long>(r.vertices) - static_cast<long long>(r.edges) +
                            static_cast<long long>(r.faces);
    r.watertight = r.edges > 0 && r.boundaryEdges == 0 && r.nonManifoldEdges == 0;
    r.consistentlyOriented = r.edges > 0 && r.inconsistentEdges == 0;

    if (r.watertight && r.consistentlyOriented) {
        // Closed orientable surface: chi = 2C - 2g  =>  g = (2C - chi) / 2
        const long long twoG = 2 * static_cast<long long>(r.components) - r.eulerCharacteristic;
        if (twoG >= 0 && twoG % 2 == 0) r.genus = twoG / 2;
    }
    return r;
}

bool rayTriangleIntersect(const Vec3& o, const Vec3& d, const Vec3& a, const Vec3& b, const Vec3& c,
                          double& t) {
    constexpr double eps = 1e-12;
    const Vec3 e1 = b - a, e2 = c - a;
    const Vec3 p = cross(d, e2);
    const double det = dot(e1, p);
    if (std::fabs(det) < eps) return false;  // ray parallel to triangle plane
    const double inv = 1.0 / det;
    const Vec3 s = o - a;
    const double u = dot(s, p) * inv;
    if (u < 0.0 || u > 1.0) return false;
    const Vec3 q = cross(s, e1);
    const double v = dot(d, q) * inv;
    if (v < 0.0 || u + v > 1.0) return false;
    const double hit = dot(e2, q) * inv;
    if (hit <= eps) return false;  // intersection behind the origin
    t = hit;
    return true;
}

bool containsPoint(const Mesh& m, const Vec3& p) {
    static const Vec3 dirs[3] = {
        normalized({0.5773502, 0.5671203, 0.5906381}),
        normalized({-0.3011921, 0.8137459, 0.4968722}),
        normalized({0.2793110, -0.4412903, 0.8535841}),
    };
    int insideVotes = 0;
    const auto& V = m.vertices();
    for (const Vec3& d : dirs) {
        int crossings = 0;
        for (const Triangle& tri : m.triangles()) {
            double t;
            if (rayTriangleIntersect(p, d, V[tri[0]], V[tri[1]], V[tri[2]], t)) ++crossings;
        }
        if (crossings % 2 == 1) ++insideVotes;
    }
    return insideVotes >= 2;
}

}  // namespace mesh
