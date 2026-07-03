// Generates the sample meshes in samples/. Usage: make_samples <output_dir> [--big]
#include <iostream>
#include <string>

#include "mesh/Primitives.hpp"
#include "mesh/STLIO.hpp"

using namespace mesh;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <output_dir> [--big]\n";
        return 1;
    }
    const std::string dir = argv[1];
    std::string err;
    bool ok = writeBinarySTL(dir + "/cube.stl", makeCube(1.0), &err);
    ok = ok && writeAsciiSTL(dir + "/cube_ascii.stl", makeCube(1.0), &err);
    ok = ok && writeBinarySTL(dir + "/torus.stl", makeTorus(3.0, 1.0, 48, 24), &err);
    if (argc > 2 && std::string(argv[2]) == "--big") {  // ~1M triangles for benchmarking
        ok = ok && writeBinarySTL(dir + "/torus_1M.stl", makeTorus(3.0, 1.0, 1000, 500), &err);
    }
    if (!ok) {
        std::cerr << "Error: " << err << "\n";
        return 2;
    }
    std::cout << "Samples written to " << dir << "\n";
    return 0;
}
