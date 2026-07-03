cmake_minimum_required(VERSION 3.15)
project(STLMeshParser LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()

option(ENABLE_SANITIZERS "Build with AddressSanitizer + UBSan (GCC/Clang)" OFF)

add_library(meshlib STATIC src/Mesh.cpp src/Analysis.cpp src/STLIO.cpp src/Primitives.cpp)
target_include_directories(meshlib PUBLIC include)

if(MSVC)
  target_compile_options(meshlib PUBLIC /W4)
else()
  target_compile_options(meshlib PUBLIC -Wall -Wextra -Wpedantic)
  if(ENABLE_SANITIZERS)
    target_compile_options(meshlib PUBLIC -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(meshlib PUBLIC -fsanitize=address,undefined)
  endif()
endif()

add_executable(stl_analyze src/main.cpp)
target_link_libraries(stl_analyze PRIVATE meshlib)

add_executable(make_samples tools/make_samples.cpp)
target_link_libraries(make_samples PRIVATE meshlib)

enable_testing()
add_executable(mesh_tests tests/test_main.cpp)
target_link_libraries(mesh_tests PRIVATE meshlib)
add_test(NAME mesh_tests COMMAND mesh_tests)
