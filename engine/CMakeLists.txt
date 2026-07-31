cmake_minimum_required(VERSION 3.20)

# ==============================================================================
# Find Vulkan SDK
# ==============================================================================

find_package(Vulkan REQUIRED)

if(NOT Vulkan_FOUND)
	message(FATAL_ERROR "Vulkan SDK not found. Please install the Vulkan SDK and set VULKAN_SDK environment variable.")
endif()

message(STATUS "Vulkan SDK found: ${Vulkan_INCLUDE_DIR}")

# ==============================================================================
# NeutrinoEngine Shared Library
# ==============================================================================

add_library(NeutrinoEngine SHARED
	core/engine.cpp
	core/logger.cpp
	platform/window.cpp
)

# Header files
target_include_directories(NeutrinoEngine
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/include
		${Vulkan_INCLUDE_DIR}
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}
)

# C++ Standard
target_compile_features(NeutrinoEngine PRIVATE cxx_std_20)

# Compiler-specific options (only for our library, not third-party)
if(MSVC)
	target_compile_options(NeutrinoEngine PRIVATE /W4 /permissive-)
else()
	target_compile_options(NeutrinoEngine PRIVATE -Wall -Wextra -Wpedantic)
	if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
		target_compile_options(NeutrinoEngine PRIVATE -Wthread-safety)
	endif()
endif()

# Link Vulkan library
target_link_libraries(NeutrinoEngine
	PRIVATE Vulkan::Vulkan
	PRIVATE glfw
)

# Vulkan RAII (C++) specific compiler definitions
target_compile_definitions(NeutrinoEngine
	PRIVATE
		VULKAN_HPP_NO_CONSTRUCTORS
		VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
)

# Platform-specific properties
if(WIN32)
	# Windows-specific DLL settings
	set_target_properties(NeutrinoEngine PROPERTIES
		WINDOWS_EXPORT_ALL_SYMBOLS ON
		SUFFIX ".dll"
	)
elseif(LINUX)
	# Linux-specific shared object settings
	set_target_properties(NeutrinoEngine PROPERTIES
		SUFFIX ".so"
		VERSION 1.0.0
		SOVERSION 1
	)
elseif(APPLE)
	# macOS-specific dylib settings
	set_target_properties(NeutrinoEngine PROPERTIES
		SUFFIX ".dylib"
		VERSION 1.0.0
	)
endif()

message(STATUS "NeutrinoEngine library configured")
