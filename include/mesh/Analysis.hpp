#pragma once
#include <cstddef>
#include <optional>

#include "mesh/Mesh.hpp"

namespace mesh {

struct BoundingBox {
    Vec3 min, max;
    Vec3 size() const { return max - min; }
    Vec3 center() const { return (min + max) * 0.5; }
};

struct TopologyReport {
    std::size_t vertices = 0;   // distinct vertices referenced by at least one triangle
    std::size_t edges = 0;      // distinct undirected edges
    std::size_t faces = 0;
    std::size_t components = 0; // connected components (union-find)
    std::size_t degenerateTriangles = 0;
    std::size_t boundaryEdges = 0;     // edges used by exactly 1 triangle (holes)
    std::size_t nonManifoldEdges = 0;  // edges used by more than 2 triangles
    std::size_t inconsistentEdges = 0; // shared edges traversed in the same direction twice
    long long eulerCharacteristic = 0; // V - E + F
    bool watertight = false;           // no boundary edges, no non-manifold edges
    bool consistentlyOriented = false; // every shared edge is traversed in opposite directions
    std::optional<long long> genus;    // total genus; only defined for closed, oriented meshes
};

BoundingBox boundingBox(const Mesh& m);

// Sum of 0.5 * |(b-a) x (c-a)| over all triangles.
double surfaceArea(const Mesh& m);

// Signed volume via the divergence theorem: sum of det[a b c] / 6 over all
// triangles (each is the signed volume of the tetrahedron formed with a reference
// point). Positive for outward-facing (CCW) normals, negative for inverted meshes.
// Computed relative to the bounding-box centre to reduce cancellation error.
double signedVolume(const Mesh& m);

// Centre of mass of the enclosed solid (uniform density).
Vec3 centroid(const Mesh& m);

TopologyReport analyzeTopology(const Mesh& m);

// Moller-Trumbore ray/triangle test. On a hit returns true and sets t (> eps).
bool rayTriangleIntersect(const Vec3& origin, const Vec3& dir, const Vec3& a, const Vec3& b,
                          const Vec3& c, double& t);

// Point-in-solid test by ray casting (odd crossings = inside). Uses a majority vote
// over three skewed rays so a ray grazing an edge/vertex does not flip the answer.
// Only meaningful for watertight meshes. O(F) per query.
bool containsPoint(const Mesh& m, const Vec3& p);

}  // namespace mesh
