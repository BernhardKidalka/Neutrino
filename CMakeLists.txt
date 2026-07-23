cmake_minimum_required(VERSION 3.20)
project(Neutrino 
	VERSION 1.0.0
	DESCRIPTION "Neutrino - Cross-platform C++ Application"
	LANGUAGES CXX)

# ==============================================================================
# Build Configuration
# ==============================================================================

# C++ Standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build Directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# ==============================================================================
# Compiler-specific Settings
# ==============================================================================

# Note: Compiler options are applied to targets individually to avoid conflicts
# with third-party libraries. This prevents D9025 warnings.

# ==============================================================================
# Platform-specific Settings
# ==============================================================================

if(WIN32)
	message(STATUS "Configure for Windows")
	add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
elseif(UNIX AND NOT APPLE)
	message(STATUS "Configure for Linux")
	set(LINUX TRUE)
elseif(APPLE)
	message(STATUS "Configure for macOS")
endif()

# ==============================================================================
# Dependencies
# ==============================================================================

# GLFW Configuration
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

add_subdirectory(third_party/glfw)

# ==============================================================================
# Subdirectories
# ==============================================================================

add_subdirectory(engine)
add_subdirectory(src)

# ==============================================================================
# Info
# ==============================================================================

message(STATUS "")
message(STATUS "Neutrino Configuration:")
message(STATUS "  Platform: ${CMAKE_SYSTEM_NAME}")
message(STATUS "  Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Output Directory: ${CMAKE_BINARY_DIR}/bin")
message(STATUS "")
