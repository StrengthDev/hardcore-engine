
add_subdirectory(device)
add_subdirectory(pipeline)
add_subdirectory(render_pass)
add_subdirectory(resource)
add_subdirectory(shader)

target_sources(${PROJECT_NAME}
    PRIVATE
        renderer.cpp
        util.cpp
)
