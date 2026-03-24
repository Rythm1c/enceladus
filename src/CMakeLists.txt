# add all source files for the executable
add_executable(enceladus
    main.cxx
    utils.cxx
    core.cxx
    shader.cxx
    pipeline.cxx
    renderpass.cxx
    renderer.cxx
)

# Link libraries
# On MinGW/SDL2 builds the SDL2main library provides the WinMain
# entry point wrapper; without it the linker will complain of an
# undefined reference to `WinMain` when targeting the Windows GUI
# subsystem.  The imported target `SDL2::SDL2` doesn't automatically
# pull in SDL2main, so we link both targets explicitly.
target_link_libraries(enceladus
    PRIVATE
        Vulkan::Vulkan
        # SDL2main depends on SDL2 symbols, so it must come *before*
        # the core SDL2 library on the command line (linker order is
        # significant when using static archives under MinGW).
        SDL2::SDL2main
        SDL2::SDL2
)

# Include directories
target_include_directories(enceladus
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}/headers
)
