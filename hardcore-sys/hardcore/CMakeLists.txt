cmake_minimum_required(VERSION 3.14)
project("hardcore" C CXX)
set(CMAKE_CXX_STANDARD 20)

add_subdirectory(thirdparty/glfw)

# Add source files to SOURCES
add_subdirectory(src)

# Setup library target
set(LIB_NAME "hardcore")
add_library(${LIB_NAME} STATIC ${SOURCES})
target_include_directories(${LIB_NAME} PUBLIC "include" PRIVATE "src" "thirdparty/glfw/include")
target_link_libraries(${PROJECT_NAME} PRIVATE glfw)
target_precompile_headers(${LIB_NAME} PRIVATE "src/pch.hpp")

install(TARGETS ${PROJECT_NAME}
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

install(DIRECTORY "include" DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/..")
