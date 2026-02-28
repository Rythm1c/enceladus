# add all source files for the executable
add_executable(enceladus
    main.cxx
    core.cxx      # core implementation must be compiled
)

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
        ${PROJECT_SOURCE_DIR}/headers   # headers location
)
