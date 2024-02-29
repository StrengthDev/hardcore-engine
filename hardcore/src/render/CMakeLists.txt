list(APPEND ENGINE_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/renderer.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/device.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/device_heap_manager.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/resource_pool.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/staging_pool.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/memory.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/swapchain.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/graphics_pipeline.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/shader.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/shader_library.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/resource.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/pipeline.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/graph.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/call_instances.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/memory_reference.cpp
)

set(ENGINE_SOURCES ${ENGINE_SOURCES} PARENT_SCOPE)
