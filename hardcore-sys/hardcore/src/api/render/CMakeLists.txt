
add_subdirectory(ops)
add_subdirectory(resource)

target_sources(${PROJECT_NAME}
    PRIVATE
        device.cpp
        renderer.cpp
        shader.cpp
)
