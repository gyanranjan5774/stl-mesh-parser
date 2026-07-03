#include "mesh/Primitives.hpp"

#include <cmath>

namespace mesh {

Mesh makeCube(double s, const Vec3& o) {
    const Vec3 v[8] = {
        o + Vec3{0, 0, 0}, o + Vec3{s, 0, 0}, o + Vec3{s, s, 0}, o + Vec3{0, s, 0},
        o + Vec3{0, 0, s}, o + Vec3{s, 0, s}, o + Vec3{s, s, s}, o + Vec3{0, s, s},
    };
    static const int f[12][3] = {
        {0, 2, 1}, {0, 3, 2},  // bottom (-z)
        {4, 5, 6}, {4, 6, 7},  // top    (+z)
        {0, 1, 5}, {0, 5, 4},  // front  (-y)
        {3, 7, 6}, {3, 6, 2},  // back   (+y)
        {0, 4, 7}, {0, 7, 3},  // left   (-x)
        {1, 2, 6}, {1, 6, 5},  // right  (+x)
    };
    Mesh m;
    m.reserve(12);
    for (const auto& t : f) m.addTriangle(v[t[0]], v[t[1]], v[t[2]]);
    return m;
}

Mesh makeTorus(double R, double r, int segU, int segV) {
    const double pi = std::acos(-1.0);
    // Positions are always computed from indices taken modulo the grid size, so the
    // seam vertices are bit-identical and weld correctly.
    auto P = [&](int i, int j) {
        i %= segU;
        j %= segV;
        const double phi = 2.0 * pi * i / segU, theta = 2.0 * pi * j / segV;
        const double ring = R + r * std::cos(theta);
        return Vec3{ring * std::cos(phi), ring * std::sin(phi), r * std::sin(theta)};
    };
    Mesh m;
    m.reserve(static_cast<std::size_t>(2) * segU * segV);
    for (int i = 0; i < segU; ++i) {
        for (int j = 0; j < segV; ++j) {
            const Vec3 p00 = P(i, j), p10 = P(i + 1, j), p01 = P(i, j + 1), p11 = P(i + 1, j + 1);
            m.addTriangle(p00, p10, p11);
            m.addTriangle(p00, p11, p01);
        }
    }
    return m;
}

}  // namespace mesh
