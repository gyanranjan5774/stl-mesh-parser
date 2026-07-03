#pragma once
#include "mesh/Mesh.hpp"

namespace mesh {

// Axis-aligned cube [origin, origin + size]^3, 12 triangles, outward CCW winding.
Mesh makeCube(double size = 1.0, const Vec3& origin = {});

// Torus around the Z axis (major radius R, tube radius r), 2*segU*segV triangles.
// Outward CCW winding. Exact volume = 2 * pi^2 * R * r^2, area = 4 * pi^2 * R * r.
Mesh makeTorus(double R, double r, int segU, int segV);

}  // namespace mesh
