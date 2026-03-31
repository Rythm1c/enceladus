# ---------------------------------------------------------------------------
# scene/CMakeLists.txt
# ---------------------------------------------------------------------------

add_library(enceladus_scene STATIC
    camera.cxx
    shape.cxx
)

target_include_directories(enceladus_scene
    PUBLIC
        headers
        ../math/headers
        ../renderer/headers
)

target_link_libraries(enceladus_scene
    PUBLIC
        enceladus_math
        enceladus_renderer
)