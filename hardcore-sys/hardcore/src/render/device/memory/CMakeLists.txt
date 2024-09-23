list(APPEND SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/heap_manager.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/resource_pool.cpp
        #        ${CMAKE_CURRENT_SOURCE_DIR}/staging_pool.cpp
)

set(SOURCES ${SOURCES} PARENT_SCOPE)
