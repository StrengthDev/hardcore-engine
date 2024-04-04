cmake_minimum_required(VERSION 3.14)
project("hardcore" C CXX)
set(CMAKE_CXX_STANDARD 20)

option(HARDCORE_HEADLESS "disable window functionality" OFF)
option(HARDCORE_LOGS "enable logging" ON)
option(HARDCORE_VALIDATION "enable Vulkan validation layers" OFF)

list(APPEND PRIVATE_INCLUDES "src")
list(APPEND LIB_SOURCES)

if (HARDCORE_HEADLESS)
    add_compile_definitions(HC_HEADLESS)
else ()
    add_subdirectory(thirdparty/glfw)
    list(APPEND PRIVATE_INCLUDES "thirdparty/glfw/include")
    list(APPEND LIB_SOURCES $<TARGET_OBJECTS:glfw>)
endif ()

if (HARDCORE_LOGS)
    add_compile_definitions(HC_LOGGING)
endif ()

if (HARDCORE_VALIDATION)
    add_compile_definitions(HC_VULKAN_VALIDATION)
endif ()

# Add source files to SOURCES
add_subdirectory(src)

# Setup library target
add_library(${PROJECT_NAME})
target_include_directories(${PROJECT_NAME} PUBLIC "include" PRIVATE ${PRIVATE_INCLUDES})
# Instead of calling target_link_libraries(${PROJECT_NAME} PRIVATE ${LIBRARIES}), the library compiled objects are
# linked directly so that everything is included in hardcore's library
target_sources(${PROJECT_NAME} PUBLIC ${SOURCES} PRIVATE ${LIB_SOURCES})
target_precompile_headers(${PROJECT_NAME} PRIVATE "src/pch.hpp")

install(TARGETS ${PROJECT_NAME}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY "include" DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/..")
