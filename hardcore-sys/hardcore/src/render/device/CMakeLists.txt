
add_subdirectory(memory)
add_subdirectory(swapchain)
add_subdirectory(graph)

target_sources(${PROJECT_NAME}
    PRIVATE
        cleaner.cpp
        device.cpp
        scheduler.cpp
)
