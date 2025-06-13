
add_subdirectory(memory)

target_sources(${PROJECT_NAME}
    PRIVATE
        device.cpp
        scheduler.cpp
        graph.cpp
        swapchain.cpp
)
