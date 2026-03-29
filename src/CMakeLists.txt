# ---------------------------------------------------------------------------
# src/CMakeLists.txt
# ---------------------------------------------------------------------------

add_executable(enceladus
    main.cxx
    buffer.cxx
    vertex.cxx
    shape.cxx
    utils.cxx
    core.cxx
    shader.cxx
    pipeline.cxx
    renderpass.cxx
    swapchain.cxx
    renderer.cxx
    camera.cxx
    descriptor.cxx
)

target_link_libraries(enceladus
    PRIVATE
        Vulkan::Vulkan
        SDL2::SDL2main   # must come before SDL2::SDL2 (linker order)
        SDL2::SDL2
        enceladus_math   # your custom math library
        imgui            # Dear ImGui (Vulkan + SDL2 backends)
)

target_include_directories(enceladus
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}/headers
        # enceladus_math and imgui expose their own include dirs via PUBLIC
        # in their own CMakeLists, so no need to repeat them here.
)