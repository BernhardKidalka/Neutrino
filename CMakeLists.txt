cmake_minimum_required(VERSION 3.20)
project(Neutrino 
	VERSION 1.0.0
	DESCRIPTION "Neutrino - Cross-platform C++ Application"
	LANGUAGES CXX)

# ==============================================================================
# Build-Konfiguration
# ==============================================================================

# C++ Standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build Verzeichnisse
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# ==============================================================================
# Compiler-spezifische Einstellungen
# ==============================================================================

if(MSVC)
	# Visual Studio Compiler
	add_compile_options(/W4)  # Warnstufe 4
	add_compile_options(/permissive-)  # Strikte Konformität
else()
	# GCC/Clang (Linux, etc.)
	add_compile_options(-Wall -Wextra -Wpedantic)
	if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
		add_compile_options(-Wthread-safety)
	endif()
endif()

# ==============================================================================
# Plattform-spezifische Einstellungen
# ==============================================================================

if(WIN32)
	message(STATUS "Konfiguriere für Windows")
	add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
elseif(UNIX AND NOT APPLE)
	message(STATUS "Konfiguriere für Linux")
	set(LINUX TRUE)
elseif(APPLE)
	message(STATUS "Konfiguriere für macOS")
endif()

# ==============================================================================
# Abhängigkeiten
# ==============================================================================

# Hier können Sie externe Bibliotheken hinzufügen (z.B. mit find_package)
# Beispiel:
# find_package(OpenGL REQUIRED)

# ==============================================================================
# Subdirectories
# ==============================================================================

add_subdirectory(engine)
add_subdirectory(src)

# ==============================================================================
# Info
# ==============================================================================

message(STATUS "")
message(STATUS "Neutrino Konfiguration:")
message(STATUS "  Platform: ${CMAKE_SYSTEM_NAME}")
message(STATUS "  Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Output Directory: ${CMAKE_BINARY_DIR}/bin")
message(STATUS "")
