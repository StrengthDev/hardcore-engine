add_subdirectory(memory)

list(APPEND SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/scheduler.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/graph.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/memory.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/swapchain.cpp
)

set(SOURCES ${SOURCES} PARENT_SCOPE)
