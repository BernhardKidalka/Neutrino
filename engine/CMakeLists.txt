cmake_minimum_required(VERSION 3.20)

# ==============================================================================
# NeutrinoEngine Shared Library
# ==============================================================================

add_library(NeutrinoEngine SHARED
	engine.cpp
)

# Header-Dateien
target_include_directories(NeutrinoEngine
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/include
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}
)

# C++ Standard
target_compile_features(NeutrinoEngine PRIVATE cxx_std_20)

# Platform-spezifische Eigenschaften
if(WIN32)
	# Windows-spezifische DLL-Einstellungen
	set_target_properties(NeutrinoEngine PROPERTIES
		WINDOWS_EXPORT_ALL_SYMBOLS ON
		SUFFIX ".dll"
	)
elseif(LINUX)
	# Linux-spezifische SO-Einstellungen
	set_target_properties(NeutrinoEngine PROPERTIES
		SUFFIX ".so"
		VERSION 1.0.0
		SOVERSION 1
	)
elseif(APPLE)
	# macOS-spezifische DYLIB-Einstellungen
	set_target_properties(NeutrinoEngine PROPERTIES
		SUFFIX ".dylib"
		VERSION 1.0.0
	)
endif()

message(STATUS "NeutrinoEngine Bibliothek konfiguriert")
