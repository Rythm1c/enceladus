# ---------------------------------------------------------------------------
# external/math/CMakeLists.txt
# Builds your custom math library as a static library target: enceladus_math
# ---------------------------------------------------------------------------

# Glob is convenient here because you control this directory and will re-run
# CMake when you add new files.  For larger projects prefer explicit lists.
file(GLOB MATH_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cxx")
file(GLOB MATH_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/*.hxx")

add_library(enceladus_math STATIC ${MATH_SOURCES} ${MATH_HEADERS})

# PUBLIC: anything that links enceladus_math automatically gets this include
# path, so they can write #include "vec3.hxx" without extra setup.
target_include_directories(enceladus_math
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(enceladus_math PUBLIC cxx_std_17)