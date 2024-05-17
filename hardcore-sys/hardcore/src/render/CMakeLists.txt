add_subdirectory(device)

list(APPEND SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/renderer.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/device.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/swapchain.cpp
)

set(SOURCES ${SOURCES} PARENT_SCOPE)
