add_subdirectory(device)
add_subdirectory(resource)
add_subdirectory(shader)

list(APPEND SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/renderer.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/device.cpp
)

set(SOURCES ${SOURCES} PARENT_SCOPE)
