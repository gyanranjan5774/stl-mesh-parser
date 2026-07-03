#pragma once
#include <string>

#include "mesh/Mesh.hpp"

namespace mesh {

enum class STLFormat { Binary, Ascii };

struct ReadResult {
    bool ok = false;
    STLFormat format = STLFormat::Binary;
    std::string error;  // human-readable reason when ok == false
};

// Reads a binary or ASCII STL file (auto-detected) into `out`.
//
// Binary detection does NOT trust the "solid" prefix (many binary exporters write
// "solid" in the 80-byte header). A file is binary iff
//     file_size == 84 + 50 * triangle_count
// which also protects against corrupt headers that claim billions of triangles.
ReadResult readSTL(const std::string& path, Mesh& out);

bool writeBinarySTL(const std::string& path, const Mesh& m, std::string* error = nullptr);
bool writeAsciiSTL(const std::string& path, const Mesh& m, std::string* error = nullptr);

}  // namespace mesh
