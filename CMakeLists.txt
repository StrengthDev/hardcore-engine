cmake_minimum_required(VERSION 3.21)

# File setting the paths for used libraries, useful for when find_package() isn't a viable option
# set GLFW_PATH
# set VULKAN_SDK_PATH
# set GLM_PATH
include(.env.cmake OPTIONAL RESULT_VARIABLE LOCAL_ENV)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

option(BUILD_SHARED_LIBS "Build shared libraries (DLLs)" ON)

# Dummy project to enable checking the C++ compiler
project(dummy LANGUAGES CXX)

if(DEFINED GLFW_PATH)
	set(GLFW_INCLUDE_DIRS "${GLFW_PATH}/include")
	if(MSVC)
		set(GLFW_LIBRARIES "${GLFW_PATH}/lib-vc2022")
	elseif(MINGW)
		set(GLFW_LIBRARIES "${GLFW_PATH}/lib-mingw-w64")
	else()
		message(FATAL_ERROR "Unknow compiler used with local environment variables")
	endif()
else()
	find_package(glfw3 3.3 REQUIRED)
endif()

if(DEFINED GLFW_PATH)
	set(Vulkan_INCLUDE_DIRS "${VULKAN_SDK_PATH}/Include")
	set(Vulkan_LIBRARIES "${VULKAN_SDK_PATH}/Lib")
else()
	find_package(Vulkan REQUIRED)
endif()

if(DEFINED GLFW_PATH)
	set(GLM_INCLUDE_DIRS "${GLM_PATH}/glm")
else()
	find_package(glm REQUIRED)
endif()

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Release")
	message(STATUS "No build type set, defaulting to Release")
endif()

if(NOT CMAKE_BINARY_DIR)
	set(CMAKE_BINARY_DIR "${CMAKE_SOURCE_DIR}/build/${CMAKE_BUILD_TYPE}")
endif()

if(CMAKE_BUILD_TYPE MATCHES "Release")
	add_compile_definitions(NDEBUG)
elseif(CMAKE_BUILD_TYPE MATCHES "Debug")
	if(MSVC)
		list(APPEND PLATFORM_COMPILE_OPTIONS "/ZI")
		list(APPEND PLATFORM_LINK_OPTIONS "/INCREMENTAL")
	endif()
else()
endif()

find_program(SPV_COMPILER glslangValidator REQUIRED)
function(add_shader TARGET SHADER)
	get_target_property(OUTPUT_DIRECTORY ${TARGET} RUNTIME_OUTPUT_DIRECTORY)
    set(SHADER_PATH "${CMAKE_CURRENT_SOURCE_DIR}/resources/shaders/${SHADER}")
    set(OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}/resources/shaders/${SHADER}.spv")
	
    get_filename_component(OUTPUT_DIR ${OUTPUT_PATH} DIRECTORY)
	add_custom_command(TARGET ${TARGET} POST_BUILD COMMAND ${CMAKE_COMMAND} -E make_directory ${OUTPUT_DIR})

    get_filename_component(SHADER_NAME ${SHADER_PATH} NAME)
	set(SHADER_TARGET_NAME "${TARGET}_${SHADER_NAME}")
	add_custom_target(${SHADER_TARGET_NAME} DEPENDS ${OUTPUT_PATH})
	add_dependencies(${TARGET} ${SHADER_TARGET_NAME})

	add_custom_command(
		COMMAND ${SPV_COMPILER} -V ${SHADER_PATH} -o ${OUTPUT_PATH}
		MAIN_DEPENDENCY ${SHADER_PATH}
		OUTPUT ${OUTPUT_PATH}
		COMMAND_EXPAND_LISTS
	)
endfunction(add_shader)

project(hardcore VERSION 0.1 LANGUAGES CXX DESCRIPTION "Hardcore engine project")
add_subdirectory(hardcore)

project(client VERSION 0.1 LANGUAGES CXX DESCRIPTION "Engine test client")
add_subdirectory(client)

project(demo VERSION 0.1 LANGUAGES CXX DESCRIPTION "Project containing several demo programs using the engine")
add_subdirectory(demo)
