
add_subdirectory(memory)
add_subdirectory(swapchain)

target_sources(${PROJECT_NAME}
    PRIVATE
        cleaner.cpp
        device.cpp
        scheduler.cpp
        graph.cpp
)
