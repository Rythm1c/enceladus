# ---------------------------------------------------------------------------
# renderer/CMakeLists.txt
# ---------------------------------------------------------------------------

add_library(enceladus_renderer STATIC
    buffer.cxx
    core.cxx
    descriptor.cxx
    pipeline.cxx
    renderer.cxx
    renderpass.cxx
    shader.cxx
    shadowmap.cxx
    swapchain.cxx
    texture.cxx
    utils.cxx
    vertex.cxx
)

target_include_directories(enceladus_renderer
    PUBLIC
        headers
)

target_link_libraries(enceladus_renderer
    PUBLIC
        Vulkan::Vulkan
        SDL2::SDL2
)