# ---------------------------------------------------------------------------
# src/CMakeLists.txt
# ---------------------------------------------------------------------------

add_executable(enceladus
    main.cxx
    scene.cxx
    application.cxx
)

target_link_libraries(enceladus
    PRIVATE
        Vulkan::Vulkan
        SDL2::SDL2
        enceladus_math   # your custom math library
        enceladus_renderer
        enceladus_scene
        enceladus_physics
        imgui            # Dear ImGui (Vulkan + SDL2 backends)
)

target_include_directories(enceladus
    PRIVATE
        ../renderer/headers
        ../scene/headers
        ../physics/headers
        
        # enceladus_math and imgui expose their own include dirs via PUBLIC
        # in their own CMakeLists, so no need to repeat them here.
)