# CV bullets (pick 3-4)

**Mesh Analysis Toolkit - STL Parser & Geometry Kernel | C++17, CMake, GitHub Actions**
* Built a dependency-free C++17 library and CLI that parses binary/ASCII STL (auto-detected, corruption-safe) and computes surface area, signed volume, centre of mass and bounding box for 3D meshes.
* Implemented vertex welding (hash map), union-find connected components and edge-map topology checks (watertight / non-manifold / winding consistency, Euler characteristic, genus).
* Validated against closed-form torus and cube results; 21 unit tests (79 checks), CI on Linux/Windows/macOS plus ASan/UBSan runs.
* Analyses a 1M-triangle (50 MB) mesh in ~0.5 s on a single thread; improved numerical accuracy by integrating relative to the bounding-box centre.

# Likely interview questions and short answers

1. **Why is volume = sum of a . (b x c) / 6?** Each triangle plus a reference point forms a tetrahedron whose signed volume is the scalar triple product / 6. For a closed, outward-oriented surface the contributions outside the solid cancel (divergence theorem), leaving the true volume.
2. **How do you know a mesh is watertight?** Every undirected edge must be shared by exactly two triangles. For consistent winding, the two triangles must traverse that edge in opposite directions.
3. **What is Euler characteristic and genus?** V - E + F = 2C - 2g for closed orientable surfaces. Cube: 2 (g = 0). Torus: 0 (g = 1).
4. **Why vertex welding and what is its complexity?** STL repeats vertices per triangle; connectivity needs shared indices. A hash map gives O(1) average insert/lookup, O(F) total.
5. **Why is union-find fast?** Path compression + union by rank gives inverse-Ackermann amortised time per operation.
6. **How does Moller-Trumbore work?** Solves o + t*d = (1-u-v)a + u*b + v*c using barycentric coordinates and Cramer's rule via two cross products, no plane precomputation.
7. **How would you speed up point-in-mesh?** Build a BVH of triangle AABBs: O(F log F) build, ~O(log F) per ray.
8. **Float vs double?** STL stores float32. Sums over millions of triangles accumulate error, so accumulate in double and translate coordinates near the centroid first.
9. **What would you do differently with more time?** BVH, tolerance-based welding via spatial hash, mesh repair, multithreading.

# Before you publish
* Run it yourself (cmake build + ctest) and be able to explain every file; interviewers will ask.
* Commit in small steps with meaningful messages instead of one big upload.
* Add a screenshot or terminal GIF of the CLI output to the README.
* Pin the repo on GitHub and put the link in your CV header next to LinkedIn.
