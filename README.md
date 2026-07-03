# 🧊 3D Mesh Analysis Toolkit — STL Parser & Geometry Engine

> **Parsing binary and ASCII STL files and analysing the 3D geometry inside them (area, volume, centre of mass, mesh quality, and topology) using modern C++17, with no external libraries.**

![C++17](https://img.shields.io/badge/C%2B%2B-17-blue)
![CMake](https://img.shields.io/badge/build-CMake-064F8C)
![CI](https://github.com/gyanranjan5774/stl-mesh-parser/actions/workflows/ci.yml/badge.svg)
![License: MIT](https://img.shields.io/badge/license-MIT-green)

---

## 📌 Project Overview

3D models move between CAD, simulation, and 3D-printing tools every day, and **STL (Stereolithography)** is one of the most widely used formats for exchanging them.

STL looks simple, but it is easy to get wrong. Files store unconnected "triangle soup", the binary and ASCII variants look alike, and real-world files can be truncated, corrupt, open (holes), or inside-out.

This project reads STL meshes and answers practical engineering questions about them: **how big is the part, how much material does it contain, where is its centre of mass, and is the mesh actually valid?**

The project follows a complete geometry-processing workflow:

```text
STL File (binary / ASCII)
        ↓
Format Detection & Validation
        ↓
Parsing (explicit byte decoding)
        ↓
Vertex Welding → Indexed Mesh
        ↓
Geometry Analysis (area, volume, centroid)
        ↓
Topology Analysis (watertight, winding, genus)
        ↓
Spatial Queries (point inside solid)
        ↓
Mesh Analysis Report
```

The objective is simple: **turn raw triangle data into clear, trustworthy measurements that support engineering decisions.**

---

# 🎯 Objectives

This project focuses on answering four key questions:

* 📂 **Can a mesh file be read safely, even when it is corrupt or truncated?**
* 📐 **What are the part's surface area, volume, and centre of mass?**
* 🧩 **Is the mesh valid: closed, consistently oriented, and free of defects?**
* 🎯 **Does a given 3D point lie inside the solid?**

---

# 🧩 Engineering Context

In CAD, CAE, and 3D-printing workflows, a mesh is only useful if it can be trusted. Before a part is simulated, printed, or measured, it has to be checked for holes, flipped faces, and numerical problems.

Raw triangle data can be difficult to reason about directly. This project transforms it into:

**Triangles → Connectivity → Measurements → Validation**

The result is a straightforward view of a model's geometry and its quality.

---

# 📊 Input Data Overview

**Source:** Binary and ASCII STL files (sample meshes included in `samples/`)

**Data Type:** Triangle mesh (3 vertices + 1 normal per triangle)

### Binary STL Layout

| Section        | Size              | Description                         |
| -------------- | ----------------- | ----------------------------------- |
| **Header**     | 80 bytes          | Free text, not used for parsing     |
| **Count**      | 4 bytes           | Number of triangles `N`             |
| **Triangles**  | 50 bytes × `N`    | Normal, 3 vertices, 2 attribute bytes |

> **Note:** STL stores each triangle independently. Shared corners are repeated, so connectivity has to be reconstructed after reading.

---

# 🛠️ Technology Stack

| Tool                  | Role in the Project                         |
| --------------------- | ------------------------------------------- |
| 💻 **C++17**          | Core library and command-line tool          |
| 🧱 **STL Containers** | `vector`, `unordered_map` for mesh storage  |
| 🏗️ **CMake**          | Cross-platform build system                 |
| 🧪 **CTest**          | Unit testing with a custom lightweight runner |
| ⚙️ **GitHub Actions** | CI on Linux, Windows, and macOS             |
| 🔍 **ASan / UBSan**   | Memory and undefined-behaviour checking     |

---

# 🧹 Data Preparation (Parsing & Validation)

The reader prepares every file before any analysis runs.

### Parsing workflow

* Detected the format from the file size: binary if `size == 84 + 50·N`, otherwise ASCII if it starts with `solid`
* Decoded every field explicitly, byte by byte, so the result does not depend on host endianness or struct padding
* Rejected NaN/Inf coordinates, truncated files, and headers that claim impossible triangle counts
* Welded duplicate vertices through a hash map to build an indexed mesh (a cube's 36 corners become 8 vertices)

After preparation, the indexed mesh is passed to the geometry and topology analysis.

---

# 🔎 Analysis Workflow

The project follows a structured geometry-processing pipeline:

### 01 — Read

Detect the format and parse the file into triangles.

### 02 — Connect

Weld shared vertices to build an indexed mesh.

### 03 — Measure

Compute surface area, signed volume, centre of mass, and bounding box.

### 04 — Validate

Check watertightness, winding consistency, non-manifold edges, and degenerate triangles.

### 05 — Query

Answer spatial questions, such as whether a point lies inside the solid.

---

# 📈 Key Metrics

The analysis reports several core mesh indicators:

### 📐 Surface Area

Sum of `½ · ‖(b − a) × (c − a)‖` over all triangles.

### 🧊 Signed Volume

Sum of `a · (b × c) / 6` over all triangles (divergence theorem). A negative value reveals inward-facing normals.

### ⚖️ Centre of Mass

Volume-weighted average of the tetrahedron centroids `(a + b + c) / 4`.

### 🧩 Euler Characteristic & Genus

`V − E + F = 2C − 2g` gives the genus `g` of a closed, orientable mesh (cube: 0, torus: 1).

### 🔗 Connected Components

Counted with a disjoint-set union (path halving, union by rank).

---

# 💡 Key Insights

Based on the analysis and the test suite:

### ✅ Results Match the Mathematics

On a torus with R = 3 and r = 1, the exact volume is `2π²Rr² ≈ 59.22` and the exact area is `4π²Rr ≈ 118.44`. A 1,000,000-triangle torus reproduces both to within about 0.1%, and the 96 × 48 torus used in the tests stays within 1%.

### 🧮 Numerical Stability Matters

Computing volume and centroid relative to the bounding-box centre keeps results correct even when a model sits 20,000 units from the origin.

### 🛑 Defects Are Detected Reliably

A deleted triangle shows up as 3 boundary edges, a flipped triangle as inconsistent winding, and an inside-out mesh as negative volume.

### 🛡️ Corrupt Files Fail Safely

Truncated files, NaN coordinates, malformed ASCII, and headers claiming billions of triangles are all rejected with a clear error, before any large allocation.

---

# 📊 Sample Output

```text
$ ./build/stl_analyze samples/torus.stl

=== Mesh Analysis Report ===
File           : samples/torus.stl (binary STL)
Triangles      : 2304
Unique vertices: 1152  (after welding)
Edges          : 3456
Components     : 1

--- Geometry ---
Bounding box   : [-4.0000, -4.0000, -1.0000] -> [4.0000, 4.0000, 1.0000]
Extents        : 8.0000 x 8.0000 x 2.0000
Surface area   : 117.8867 units^2
Signed volume  : 58.3764 units^3
Center of mass : [0.0000, 0.0000, 0.0000]

--- Topology / quality ---
Euler char.    : 0  (V - E + F)
Watertight     : yes  (boundary edges: 0, non-manifold edges: 0)
Consistent winding: yes  (inconsistent edges: 0)
Degenerate tris: 0
Genus (total)  : 1
```

The 2,304-triangle torus is slightly smaller than the exact values because flat triangles approximate a curved surface. The gap shrinks as the resolution grows.

---

# ⚙️ Getting Started

**Requirements:** a C++17 compiler (GCC, Clang, or MSVC) and CMake 3.15+.

```bash
git clone https://github.com/gyanranjan5774/stl-mesh-parser.git
cd stl-mesh-parser

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

### ▶️ Run the analyser

```bash
./build/stl_analyze samples/cube.stl
./build/stl_analyze samples/cube.stl --point 0.5 0.5 0.5   # is this point inside the solid?
```

### 🧪 Generate sample meshes

```bash
./build/make_samples samples            # cube, ASCII cube, torus
./build/make_samples samples --big      # also a 1,000,000-triangle torus
```

### 🔍 Optional: sanitizer build

```bash
cmake -S . -B build-san -DCMAKE_BUILD_TYPE=Debug -DENABLE_SANITIZERS=ON
cmake --build build-san && ctest --test-dir build-san
```

---

# 🧪 Testing

`ctest` runs **21 tests (79 checks)** with no external framework:

| Category                | What is verified                                                                                                         |
| ----------------------- | ------------------------------------------------------------------------------------------------------------------------ |
| ✅ **Known answers**    | Cube area/volume/centroid; torus volume and area vs closed-form formulas; Euler characteristic 2 (cube) and 0 (torus)    |
| 🛑 **Defects**          | Deleted triangle, flipped triangle, inside-out mesh, degenerate triangles                                                |
| 🧱 **Components**       | Two disjoint cubes → 2 components, χ = 4, volume 2                                                                       |
| 🔢 **Numerical stability** | A cube translated far from the origin still has the correct volume                                                    |
| 📂 **File handling**    | Binary/ASCII round-trips; binary header starting with `solid`; truncated, corrupt, NaN, malformed, and missing files     |

GitHub Actions builds and tests on Linux, Windows, and macOS, and also runs the sanitizer build.

---

# 🧠 Design Decisions

* **Explicit byte decoding instead of `#pragma pack` + `read()` into a struct:** independent of host endianness and struct padding, and each float can be validated.
* **`float` input, `double` accumulation:** sums over millions of triangles accumulate rounding error.
* **Integration relative to the bounding-box centre:** avoids cancellation error for meshes far from the origin.
* **Signed volume is never wrapped in `abs()`:** a negative result is a useful signal that the normals point inward.
* **Exact-float vertex welding:** fast and predictable; tolerance-based welding is listed under future work.

---

# 🏗️ End-to-End Project Architecture

```text
                 📂 STL File (binary / ASCII)
                            │
                            ▼
                  🔍 Format Detection
                   size == 84 + 50·N ?
                            │
                            ▼
                 🧱 Parser (STLIO)
              byte decoding + validation
                            │
                            ▼
                 🔗 Mesh (vertex welding)
                    indexed triangles
                            │
              ┌─────────────┴─────────────┐
              ▼                           ▼
      📐 Geometry Analysis         🧩 Topology Analysis
   area · volume · centroid     edges · components · genus
              │                           │
              └─────────────┬─────────────┘
                            ▼
                  🎯 Spatial Queries
                   point in solid
                            │
                            ▼
                📋 Mesh Analysis Report
```

### 📁 Project Structure

```text
stl-mesh-parser/
│
├── CMakeLists.txt
├── include/mesh/
│   ├── Vec3.hpp            # 3D vector type (dot, cross, normalise)
│   ├── Mesh.hpp            # Indexed triangle mesh with vertex welding
│   ├── Analysis.hpp        # Area, volume, centroid, topology, ray casting
│   ├── STLIO.hpp           # STL reader (auto-detect) and writers
│   └── Primitives.hpp      # Cube and torus generators
├── src/                    # Implementations + main.cpp (CLI)
├── tools/
│   └── make_samples.cpp    # Sample mesh generator
├── tests/
│   └── test_main.cpp       # Unit tests (no external framework)
├── samples/                # cube.stl, cube_ascii.stl, torus.stl
└── .github/workflows/
    └── ci.yml
```

---

# 🌟 What This Project Demonstrates

This project showcases practical software-engineering and geometry skills across the full pipeline:

* 💻 **Modern C++17** library design
* 🧱 **Data structures:** hash maps, disjoint-set union, edge maps
* 📐 **3D vector math:** cross products, scalar triple products, ray-triangle intersection
* 🧮 **Numerical robustness:** precision-aware accumulation
* 📂 **Binary file-format handling** and defensive input validation
* 🧩 **Computational geometry:** topology, Euler characteristic, genus
* 🧪 **Testing:** unit tests validated against closed-form maths
* ⚙️ **Engineering practice:** CMake, CI, sanitizers

---

# 🔮 Future Work

* 🌳 Replace brute-force point containment with an AABB tree (BVH) for roughly O(log F) queries
* 🧲 Tolerance-based vertex welding with a spatial hash grid
* 🩹 Mesh repair: fix winding, fill holes
* 🧵 Multithreading (OpenMP) for the per-triangle reduction loops
* 📦 Additional formats (OBJ, 3MF)

---

# ✅ Final Takeaway

A 3D model can be understood through more than its appearance: its **area, volume, centre of mass, connectivity, and mesh quality** all tell an engineer whether the part can be trusted.

This project demonstrates how to take a raw binary file, parse it safely, rebuild its structure, measure it accurately, and validate it, all in clean C++ with no external dependencies.

---

## 👨‍💻 Author

**Gyanranjan**

GitHub: [@gyanranjan5774](https://github.com/gyanranjan5774)