add_executable(enceladus main.cxx)

# Link libraries
target_link_libraries(enceladus
    PRIVATE
        Vulkan::Vulkan
        SDL2::SDL2
)

# Include directories
target_include_directories(enceladus
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src
)
