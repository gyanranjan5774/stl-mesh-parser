// Dependency-free unit tests (no framework needed). Run via `ctest` or ./mesh_tests
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

#include "mesh/Analysis.hpp"
#include "mesh/Primitives.hpp"
#include "mesh/STLIO.hpp"

using namespace mesh;

static int g_checks = 0, g_failed = 0;

#define CHECK(cond)                                                                      \
    do {                                                                                 \
        ++g_checks;                                                                      \
        if (!(cond)) {                                                                   \
            ++g_failed;                                                                  \
            std::cerr << "  FAIL " << __FILE__ << ":" << __LINE__ << "  " << #cond << "\n"; \
        }                                                                                \
    } while (0)

#define CHECK_NEAR(a, b, tol) CHECK(std::fabs((a) - (b)) <= (tol))

struct TestCase {
    std::string name;
    std::function<void()> fn;
};
static std::vector<TestCase>& registry() {
    static std::vector<TestCase> r;
    return r;
}
struct Registrar {
    Registrar(const char* n, std::function<void()> f) { registry().push_back({n, std::move(f)}); }
};
#define TEST(name)                                  \
    static void name();                             \
    static Registrar reg_##name(#name, name);       \
    static void name()

// ---------- helpers ----------
using Soup = std::vector<std::array<Vec3, 3>>;

static Soup toSoup(const Mesh& m) {
    Soup s;
    for (std::size_t i = 0; i < m.triangleCount(); ++i) s.push_back(m.corners(i));
    return s;
}
static Mesh fromSoup(const Soup& s) {
    Mesh m;
    for (const auto& t : s) m.addTriangle(t[0], t[1], t[2]);
    return m;
}
static std::string tmpPath(const std::string& name) {
    return (std::filesystem::temp_directory_path() / ("meshtest_" + name)).string();
}
static void writeBytes(const std::string& path, const std::vector<char>& bytes) {
    std::ofstream f(path, std::ios::binary);
    f.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
}
static std::vector<char> readBytes(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    return std::vector<char>((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}
static const double kPi = std::acos(-1.0);

// ---------- geometry ----------
TEST(cube_area_volume_centroid) {
    Mesh m = makeCube(1.0);
    CHECK(m.triangleCount() == 12);
    CHECK(m.vertexCount() == 8);  // welding collapses 36 corners to 8
    CHECK_NEAR(surfaceArea(m), 6.0, 1e-12);
    CHECK_NEAR(signedVolume(m), 1.0, 1e-12);
    const Vec3 c = centroid(m);
    CHECK_NEAR(c.x, 0.5, 1e-12);
    CHECK_NEAR(c.y, 0.5, 1e-12);
    CHECK_NEAR(c.z, 0.5, 1e-12);
}

TEST(cube_scaled_and_far_from_origin) {
    Mesh m = makeCube(2.0, {10000.0, -20000.0, 5000.0});
    CHECK_NEAR(surfaceArea(m), 24.0, 1e-6);
    CHECK_NEAR(signedVolume(m), 8.0, 1e-6);  // naive origin-based sum loses precision here
    const Vec3 c = centroid(m);
    CHECK_NEAR(c.x, 10001.0, 1e-6);
    CHECK_NEAR(c.y, -19999.0, 1e-6);
    CHECK_NEAR(c.z, 5001.0, 1e-6);
}

TEST(bounding_box) {
    const BoundingBox bb = boundingBox(makeCube(3.0, {1, 2, 3}));
    CHECK_NEAR(bb.min.x, 1, 1e-12);
    CHECK_NEAR(bb.max.z, 6, 1e-12);
    CHECK_NEAR(bb.size().y, 3, 1e-12);
}

TEST(inverted_mesh_has_negative_volume) {
    Soup s = toSoup(makeCube(1.0));
    for (auto& t : s) std::swap(t[1], t[2]);  // flip every triangle
    Mesh m = fromSoup(s);
    CHECK_NEAR(signedVolume(m), -1.0, 1e-12);
    const TopologyReport r = analyzeTopology(m);
    CHECK(r.watertight && r.consistentlyOriented);  // still a valid closed mesh, just inside-out
}

TEST(torus_matches_analytic_formulas) {
    const double R = 3.0, r = 1.0;
    Mesh m = makeTorus(R, r, 96, 48);
    CHECK(m.triangleCount() == 2u * 96 * 48);
    CHECK(m.vertexCount() == 96u * 48);
    const double exactV = 2 * kPi * kPi * R * r * r, exactA = 4 * kPi * kPi * R * r;
    CHECK(std::fabs(signedVolume(m) - exactV) / exactV < 0.01);
    CHECK(std::fabs(surfaceArea(m) - exactA) / exactA < 0.01);
    const Vec3 c = centroid(m);  // symmetric about the origin
    CHECK_NEAR(c.x, 0.0, 1e-9);
    CHECK_NEAR(c.y, 0.0, 1e-9);
    CHECK_NEAR(c.z, 0.0, 1e-9);
}

// ---------- topology ----------
TEST(cube_topology) {
    const TopologyReport r = analyzeTopology(makeCube(1.0));
    CHECK(r.vertices == 8 && r.edges == 18 && r.faces == 12);
    CHECK(r.eulerCharacteristic == 2);
    CHECK(r.watertight && r.consistentlyOriented);
    CHECK(r.components == 1);
    CHECK(r.degenerateTriangles == 0);
    CHECK(r.genus.has_value() && *r.genus == 0);
}

TEST(torus_topology_genus_one) {
    const TopologyReport r = analyzeTopology(makeTorus(3.0, 1.0, 24, 12));
    CHECK(r.eulerCharacteristic == 0);
    CHECK(r.watertight && r.consistentlyOriented);
    CHECK(r.genus.has_value() && *r.genus == 1);
}

TEST(missing_triangle_creates_hole) {
    Soup s = toSoup(makeCube(1.0));
    s.pop_back();
    const TopologyReport r = analyzeTopology(fromSoup(s));
    CHECK(!r.watertight);
    CHECK(r.boundaryEdges == 3);  // one triangle removed -> its 3 edges become open
    CHECK(!r.genus.has_value());
}

TEST(flipped_triangle_breaks_orientation) {
    Soup s = toSoup(makeCube(1.0));
    std::swap(s[0][1], s[0][2]);
    const TopologyReport r = analyzeTopology(fromSoup(s));
    CHECK(r.watertight);
    CHECK(!r.consistentlyOriented);
    CHECK(r.inconsistentEdges == 3);
}

TEST(two_disjoint_cubes_are_two_components) {
    Soup s = toSoup(makeCube(1.0));
    Soup b = toSoup(makeCube(1.0, {5, 0, 0}));
    s.insert(s.end(), b.begin(), b.end());
    Mesh m = fromSoup(s);
    const TopologyReport r = analyzeTopology(m);
    CHECK(r.components == 2);
    CHECK(r.eulerCharacteristic == 4);
    CHECK(r.genus.has_value() && *r.genus == 0);
    CHECK_NEAR(signedVolume(m), 2.0, 1e-9);
}

TEST(degenerate_triangle_detected) {
    Soup s = toSoup(makeCube(1.0));
    s.push_back({Vec3{0, 0, 0}, Vec3{1, 0, 0}, Vec3{2, 0, 0}});  // collinear points
    const TopologyReport r = analyzeTopology(fromSoup(s));
    CHECK(r.degenerateTriangles == 1);
}

// ---------- ray casting ----------
TEST(ray_triangle_hit_and_miss) {
    const Vec3 a{0, 0, 0}, b{1, 0, 0}, c{0, 1, 0};
    double t = 0;
    CHECK(rayTriangleIntersect({0.25, 0.25, 1}, {0, 0, -1}, a, b, c, t));
    CHECK_NEAR(t, 1.0, 1e-12);
    CHECK(!rayTriangleIntersect({2, 2, 1}, {0, 0, -1}, a, b, c, t));    // outside triangle
    CHECK(!rayTriangleIntersect({0.25, 0.25, 1}, {0, 0, 1}, a, b, c, t));  // pointing away
    CHECK(!rayTriangleIntersect({0.25, 0.25, 1}, {1, 0, 0}, a, b, c, t));  // parallel
}

TEST(point_in_mesh) {
    Mesh cube = makeCube(1.0);
    CHECK(containsPoint(cube, {0.5, 0.5, 0.5}));
    CHECK(containsPoint(cube, {0.1, 0.9, 0.2}));
    CHECK(!containsPoint(cube, {2, 2, 2}));
    CHECK(!containsPoint(cube, {0.5, 0.5, 1.5}));
    CHECK(!containsPoint(cube, {-0.2, 0.5, 0.5}));
    Mesh torus = makeTorus(3.0, 1.0, 48, 24);
    CHECK(!containsPoint(torus, {0, 0, 0}));  // the hole of the torus is outside the solid
    CHECK(containsPoint(torus, {3, 0, 0}));   // centre of the tube is inside
}

// ---------- STL I/O ----------
TEST(binary_roundtrip) {
    const std::string p = tmpPath("rt_bin.stl");
    Mesh src = makeTorus(3.0, 1.0, 16, 8);
    CHECK(writeBinarySTL(p, src));
    Mesh dst;
    ReadResult rr = readSTL(p, dst);
    CHECK(rr.ok && rr.format == STLFormat::Binary);
    CHECK(dst.triangleCount() == src.triangleCount());
    CHECK(dst.vertexCount() == src.vertexCount());
    CHECK_NEAR(signedVolume(dst), signedVolume(src), 1e-4);
    CHECK(std::filesystem::file_size(p) == 84u + 50u * src.triangleCount());
    std::filesystem::remove(p);
}

TEST(ascii_roundtrip_and_autodetect) {
    const std::string p = tmpPath("rt_ascii.stl");
    CHECK(writeAsciiSTL(p, makeCube(1.0)));
    Mesh m;
    ReadResult rr = readSTL(p, m);
    CHECK(rr.ok && rr.format == STLFormat::Ascii);
    CHECK(m.triangleCount() == 12);
    CHECK_NEAR(signedVolume(m), 1.0, 1e-6);
    std::filesystem::remove(p);
}

TEST(binary_file_starting_with_solid_is_still_binary) {
    const std::string p = tmpPath("solidhdr.stl");
    CHECK(writeBinarySTL(p, makeCube(1.0)));
    std::vector<char> bytes = readBytes(p);
    const char tag[] = "solid exported-by-cad-tool";
    std::copy(tag, tag + sizeof tag - 1, bytes.begin());  // overwrite header text
    writeBytes(p, bytes);
    Mesh m;
    ReadResult rr = readSTL(p, m);
    CHECK(rr.ok && rr.format == STLFormat::Binary);
    CHECK(m.triangleCount() == 12);
    std::filesystem::remove(p);
}

TEST(truncated_binary_is_rejected) {
    const std::string p = tmpPath("trunc.stl");
    CHECK(writeBinarySTL(p, makeCube(1.0)));
    std::vector<char> bytes = readBytes(p);
    bytes.resize(bytes.size() - 20);
    writeBytes(p, bytes);
    Mesh m;
    ReadResult rr = readSTL(p, m);
    CHECK(!rr.ok);
    CHECK(!rr.error.empty());
    std::filesystem::remove(p);
}

TEST(huge_triangle_count_in_header_does_not_allocate) {
    const std::string p = tmpPath("hugecount.stl");
    std::vector<char> bytes(84 + 50, 0);
    bytes[80] = bytes[81] = bytes[82] = bytes[83] = static_cast<char>(0xFF);  // claims ~4.29e9 triangles
    writeBytes(p, bytes);
    Mesh m;
    ReadResult rr = readSTL(p, m);
    CHECK(!rr.ok);
    CHECK(m.triangleCount() == 0);
    std::filesystem::remove(p);
}

TEST(garbage_and_missing_files_are_rejected) {
    const std::string p = tmpPath("garbage.stl");
    writeBytes(p, std::vector<char>(200, 'x'));
    Mesh m;
    CHECK(!readSTL(p, m).ok);
    std::filesystem::remove(p);
    CHECK(!readSTL(tmpPath("does_not_exist.stl"), m).ok);
}

TEST(nan_coordinate_is_rejected) {
    const std::string p = tmpPath("nan.stl");
    CHECK(writeBinarySTL(p, makeCube(1.0)));
    std::vector<char> bytes = readBytes(p);
    const unsigned char nanLE[4] = {0x00, 0x00, 0xC0, 0x7F};  // quiet NaN
    for (int i = 0; i < 4; ++i) bytes[84 + 12 + i] = static_cast<char>(nanLE[i]);  // v1.x of triangle 0
    writeBytes(p, bytes);
    Mesh m;
    CHECK(!readSTL(p, m).ok);
    std::filesystem::remove(p);
}

TEST(malformed_ascii_is_rejected) {
    const std::string p = tmpPath("bad_ascii.stl");
    {
        std::ofstream f(p);
        f << "solid x\nfacet normal 0 0 1\nouter loop\nvertex 0 0 0\nvertex 1 0 0\nendloop\nendfacet\nendsolid x\n";
    }
    Mesh m;
    CHECK(!readSTL(p, m).ok);  // only 2 vertices in the facet
    std::filesystem::remove(p);
}

int main() {
    int failedTests = 0;
    for (const auto& t : registry()) {
        const int before = g_failed;
        t.fn();
        const bool ok = g_failed == before;
        if (!ok) ++failedTests;
        std::printf("[%s] %s\n", ok ? " OK " : "FAIL", t.name.c_str());
    }
    std::printf("\n%zu tests, %d checks, %d failed checks\n", registry().size(), g_checks, g_failed);
    return g_failed == 0 ? 0 : 1;
}
