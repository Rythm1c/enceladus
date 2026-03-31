# ---------------------------------------------------------------------------
# physics/CMakeLists.txt
# ---------------------------------------------------------------------------

# Physics library - currently empty, will be populated with physics code
add_library(enceladus_physics INTERFACE)

target_include_directories(enceladus_physics
    INTERFACE
        headers
)