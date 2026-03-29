# ---------------------------------------------------------------------------
# external/imgui/CMakeLists.txt
# Builds Dear ImGui as a static library with the Vulkan + SDL2 backends.
#
# Expected files in this directory (copy from the imgui repo):
#   Core:
#     imgui.cpp / imgui.h
#     imgui_draw.cpp
#     imgui_tables.cpp
#     imgui_widgets.cpp
#     imgui_demo.cpp          (optional — remove entry below if not wanted)
#     imconfig.h
#     imgui_internal.h
#     imstb_rectpack.h
#     imstb_textedit.h
#     imstb_truetype.h
#   Backends:
#     backends/imgui_impl_vulkan.cpp / .h
#     backends/imgui_impl_sdl2.cpp  / .h
# ---------------------------------------------------------------------------

add_library(imgui STATIC
    # Core
    imgui.cpp
    imgui_draw.cpp
    imgui_tables.cpp
    imgui_widgets.cpp
    imgui_demo.cpp          # remove this line if you don't need the demo window

    # Backends
    imgui_impl_vulkan.cpp
    imgui_impl_sdl2.cpp
)

# imgui needs to find its own headers and the backends subfolder
target_include_directories(imgui
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}           # imgui.h, imconfig.h, etc.
        ${CMAKE_CURRENT_SOURCE_DIR}/backends  # imgui_impl_vulkan.h, imgui_impl_sdl2.h
)

# imgui's Vulkan backend needs the Vulkan headers; SDL2 backend needs SDL2.
# PRIVATE here because consumers of the imgui target don't need to see
# Vulkan/SDL2 headers just because they link imgui — they pull those in
# themselves via their own target_link_libraries.
target_link_libraries(imgui
    PRIVATE
        Vulkan::Vulkan
        SDL2::SDL2
)

target_compile_features(imgui PUBLIC cxx_std_17)