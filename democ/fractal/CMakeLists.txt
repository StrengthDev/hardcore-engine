add_executable(fractal
main.cpp
layer.cpp
)

target_include_directories(fractal PRIVATE "include")
target_link_libraries(fractal ${ENGINE})
target_compile_options(fractal PUBLIC ${PROJECT_COMPILE_OPTIONS})
target_link_options(fractal PUBLIC "LINKER:${PROJECT_LINK_OPTIONS_STRING}")

add_custom_command(TARGET fractal POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_RUNTIME_DLLS:fractal> $<TARGET_FILE_DIR:fractal>
  COMMAND_EXPAND_LISTS
)

add_shader(fractal "shader.vert")
add_shader(fractal "shader.frag")
