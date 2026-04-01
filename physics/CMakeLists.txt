# ---------------------------------------------------------------------------
# physics/CMakeLists.txt
# ---------------------------------------------------------------------------

# Physics library - currently empty, will be populated with physics code
add_library(enceladus_physics STATIC
    rigidbody.cxx
    
)

target_include_directories(enceladus_physics
    INTERFACE
        headers
        ../math/headers
)
target_link_libraries(enceladus_scene
    PUBLIC
        enceladus_math
)