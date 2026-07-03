#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>

#include "mesh/Analysis.hpp"
#include "mesh/STLIO.hpp"

using namespace mesh;

static void usage(const char* prog) {
    std::cout << "Usage: " << prog << " <file.stl> [--point X Y Z]\n"
              << "  Analyses a binary or ASCII STL mesh (geometry + topology).\n"
              << "  --point X Y Z   also test whether the point lies inside the solid\n";
}

static const char* yesno(bool b) { return b ? "yes" : "NO"; }

int main(int argc, char* argv[]) {
    if (argc < 2) {
        usage(argv[0]);
        return 1;
    }
    const std::string path = argv[1];

    bool testPoint = false;
    Vec3 query;
    if (argc >= 6 && std::string(argv[2]) == "--point") {
        testPoint = true;
        query = {std::atof(argv[3]), std::atof(argv[4]), std::atof(argv[5])};
    } else if (argc > 2) {
        usage(argv[0]);
        return 1;
    }

    Mesh mesh;
    const ReadResult rr = readSTL(path, mesh);
    if (!rr.ok) {
        std::cerr << "Error: " << rr.error << "\n";
        return 2;
    }

    const TopologyReport topo = analyzeTopology(mesh);
    const BoundingBox bb = boundingBox(mesh);
    const Vec3 size = bb.size();
    const double volume = signedVolume(mesh);
    Vec3 com = centroid(mesh);
    auto clean = [](double v) { return std::fabs(v) < 5e-5 ? 0.0 : v; };  // avoid printing "-0.0000"
    com = {clean(com.x), clean(com.y), clean(com.z)};

    std::cout << std::fixed << std::setprecision(4);
    std::cout << "=== Mesh Analysis Report ===\n";
    std::cout << "File           : " << path << " (" << (rr.format == STLFormat::Binary ? "binary" : "ASCII") << " STL)\n";
    std::cout << "Triangles      : " << topo.faces << "\n";
    std::cout << "Unique vertices: " << topo.vertices << "  (after welding)\n";
    std::cout << "Edges          : " << topo.edges << "\n";
    std::cout << "Components     : " << topo.components << "\n";
    std::cout << "\n--- Geometry ---\n";
    std::cout << "Bounding box   : [" << bb.min.x << ", " << bb.min.y << ", " << bb.min.z << "] -> ["
              << bb.max.x << ", " << bb.max.y << ", " << bb.max.z << "]\n";
    std::cout << "Extents        : " << size.x << " x " << size.y << " x " << size.z << "\n";
    std::cout << "Surface area   : " << surfaceArea(mesh) << " units^2\n";
    std::cout << "Signed volume  : " << volume << " units^3\n";
    std::cout << "Center of mass : [" << com.x << ", " << com.y << ", " << com.z << "]\n";
    std::cout << "\n--- Topology / quality ---\n";
    std::cout << "Euler char.    : " << topo.eulerCharacteristic << "  (V - E + F)\n";
    std::cout << "Watertight     : " << yesno(topo.watertight) << "  (boundary edges: " << topo.boundaryEdges
              << ", non-manifold edges: " << topo.nonManifoldEdges << ")\n";
    std::cout << "Consistent winding: " << yesno(topo.consistentlyOriented)
              << "  (inconsistent edges: " << topo.inconsistentEdges << ")\n";
    std::cout << "Degenerate tris: " << topo.degenerateTriangles << "\n";
    if (topo.genus) std::cout << "Genus (total)  : " << *topo.genus << "\n";
    if (topo.watertight && topo.consistentlyOriented && volume < 0) {
        std::cout << "WARNING        : negative volume - triangle normals point inward\n";
    }
    if (!topo.watertight) {
        std::cout << "NOTE           : mesh is not closed; volume and centroid are not reliable\n";
    }
    if (testPoint) {
        std::cout << "\nPoint [" << query.x << ", " << query.y << ", " << query.z << "] is "
                  << (containsPoint(mesh, query) ? "INSIDE" : "OUTSIDE") << " the mesh\n";
    }
    return 0;
}
