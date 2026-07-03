#include "mesh/STLIO.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iomanip>

#include "mesh/Analysis.hpp"

namespace mesh {

namespace {

constexpr std::uint64_t kHeaderBytes = 80;
constexpr std::uint64_t kCountBytes = 4;
constexpr std::uint64_t kRecordBytes = 50;  // 12 normal + 36 vertices + 2 attribute

// STL is little-endian by specification. Decoding byte-by-byte keeps this correct on
// any host endianness and avoids alignment / struct-padding assumptions entirely.
std::uint32_t readLE32(const unsigned char* p) {
    return static_cast<std::uint32_t>(p[0]) | (static_cast<std::uint32_t>(p[1]) << 8) |
           (static_cast<std::uint32_t>(p[2]) << 16) | (static_cast<std::uint32_t>(p[3]) << 24);
}

float readLEFloat(const unsigned char* p) {
    const std::uint32_t u = readLE32(p);
    float f;
    std::memcpy(&f, &u, sizeof f);
    return f;
}

void writeLE32(unsigned char* p, std::uint32_t v) {
    p[0] = static_cast<unsigned char>(v & 0xFF);
    p[1] = static_cast<unsigned char>((v >> 8) & 0xFF);
    p[2] = static_cast<unsigned char>((v >> 16) & 0xFF);
    p[3] = static_cast<unsigned char>((v >> 24) & 0xFF);
}

void writeLEFloat(unsigned char* p, float f) {
    std::uint32_t u;
    std::memcpy(&u, &f, sizeof u);
    writeLE32(p, u);
}

ReadResult fail(std::string msg) {
    ReadResult r;
    r.ok = false;
    r.error = std::move(msg);
    return r;
}

ReadResult readBinary(std::ifstream& in, std::uint32_t count, Mesh& out) {
    in.seekg(static_cast<std::streamoff>(kHeaderBytes + kCountBytes), std::ios::beg);
    out.reserve(count);
    unsigned char rec[kRecordBytes];
    for (std::uint32_t i = 0; i < count; ++i) {
        if (!in.read(reinterpret_cast<char*>(rec), kRecordBytes)) {
            return fail("unexpected end of file at triangle " + std::to_string(i));
        }
        Vec3 v[3];
        for (int k = 0; k < 3; ++k) {
            const unsigned char* p = rec + 12 + 12 * k;  // skip the stored normal
            v[k] = {readLEFloat(p), readLEFloat(p + 4), readLEFloat(p + 8)};
            if (!std::isfinite(v[k].x) || !std::isfinite(v[k].y) || !std::isfinite(v[k].z)) {
                return fail("non-finite coordinate (NaN/Inf) in triangle " + std::to_string(i));
            }
        }
        out.addTriangle(v[0], v[1], v[2]);
    }
    ReadResult r;
    r.ok = true;
    r.format = STLFormat::Binary;
    return r;
}

ReadResult readAscii(const std::string& path, Mesh& out) {
    std::ifstream in(path);
    if (!in) return fail("cannot open file: " + path);
    std::string tok;
    Vec3 pending[3];
    int n = 0;
    std::size_t facets = 0;
    while (in >> tok) {
        std::transform(tok.begin(), tok.end(), tok.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        if (tok == "vertex") {
            double x, y, z;
            if (!(in >> x >> y >> z)) return fail("malformed 'vertex' line near facet " + std::to_string(facets));
            if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z)) {
                return fail("non-finite coordinate near facet " + std::to_string(facets));
            }
            if (n == 3) return fail("facet " + std::to_string(facets) + " has more than 3 vertices");
            pending[n++] = {x, y, z};
        } else if (tok == "endfacet") {
            if (n != 3) return fail("facet " + std::to_string(facets) + " does not have exactly 3 vertices");
            out.addTriangle(pending[0], pending[1], pending[2]);
            n = 0;
            ++facets;
        }
    }
    if (n != 0) return fail("file ends in the middle of a facet");
    if (facets == 0) return fail("no facets found");
    ReadResult r;
    r.ok = true;
    r.format = STLFormat::Ascii;
    return r;
}

bool setError(std::string* error, const std::string& msg) {
    if (error) *error = msg;
    return false;
}

}  // namespace

ReadResult readSTL(const std::string& path, Mesh& out) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return fail("cannot open file: " + path);

    in.seekg(0, std::ios::end);
    const std::uint64_t size = static_cast<std::uint64_t>(in.tellg());
    in.seekg(0, std::ios::beg);

    if (size >= kHeaderBytes + kCountBytes) {
        unsigned char countBytes[4];
        in.seekg(static_cast<std::streamoff>(kHeaderBytes), std::ios::beg);
        in.read(reinterpret_cast<char*>(countBytes), 4);
        const std::uint32_t count = readLE32(countBytes);
        if (size == kHeaderBytes + kCountBytes + kRecordBytes * count) {
            if (count == 0) return fail("binary STL contains zero triangles");
            return readBinary(in, count, out);
        }
    }

    // Not a size-consistent binary file: it must be ASCII to be valid.
    in.clear();
    in.seekg(0, std::ios::beg);
    char head[5] = {0, 0, 0, 0, 0};
    in.read(head, 5);
    std::string h(head, 5);
    std::transform(h.begin(), h.end(), h.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    in.close();
    if (h == "solid") return readAscii(path, out);

    if (size >= kHeaderBytes + kCountBytes) {
        return fail("file size does not match the triangle count in the binary header (truncated or corrupt)");
    }
    return fail("file is too small to be a valid STL");
}

bool writeBinarySTL(const std::string& path, const Mesh& m, std::string* error) {
    std::ofstream out(path, std::ios::binary);
    if (!out) return setError(error, "cannot open for writing: " + path);

    char header[kHeaderBytes] = {};
    const char label[] = "stl-mesh-parser binary STL";
    std::memcpy(header, label, sizeof label - 1);
    out.write(header, kHeaderBytes);

    unsigned char buf[kRecordBytes];
    writeLE32(buf, static_cast<std::uint32_t>(m.triangleCount()));
    out.write(reinterpret_cast<char*>(buf), 4);

    for (std::size_t i = 0; i < m.triangleCount(); ++i) {
        const auto c = m.corners(i);
        const Vec3 n = normalized(cross(c[1] - c[0], c[2] - c[0]));
        std::memset(buf, 0, sizeof buf);
        writeLEFloat(buf + 0, static_cast<float>(n.x));
        writeLEFloat(buf + 4, static_cast<float>(n.y));
        writeLEFloat(buf + 8, static_cast<float>(n.z));
        for (int k = 0; k < 3; ++k) {
            writeLEFloat(buf + 12 + 12 * k, static_cast<float>(c[k].x));
            writeLEFloat(buf + 16 + 12 * k, static_cast<float>(c[k].y));
            writeLEFloat(buf + 20 + 12 * k, static_cast<float>(c[k].z));
        }
        out.write(reinterpret_cast<char*>(buf), kRecordBytes);  // last 2 bytes = attribute count (0)
    }
    return out.good() ? true : setError(error, "write failed: " + path);
}

bool writeAsciiSTL(const std::string& path, const Mesh& m, std::string* error) {
    std::ofstream out(path);
    if (!out) return setError(error, "cannot open for writing: " + path);
    out << std::setprecision(9);
    out << "solid mesh\n";
    for (std::size_t i = 0; i < m.triangleCount(); ++i) {
        const auto c = m.corners(i);
        const Vec3 n = normalized(cross(c[1] - c[0], c[2] - c[0]));
        out << "  facet normal " << n.x << ' ' << n.y << ' ' << n.z << "\n    outer loop\n";
        for (int k = 0; k < 3; ++k) out << "      vertex " << c[k].x << ' ' << c[k].y << ' ' << c[k].z << '\n';
        out << "    endloop\n  endfacet\n";
    }
    out << "endsolid mesh\n";
    return out.good() ? true : setError(error, "write failed: " + path);
}

}  // namespace mesh
