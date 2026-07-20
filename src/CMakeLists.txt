cmake_minimum_required(VERSION 3.20)

# ==============================================================================
# Neutrino Executable
# ==============================================================================

add_executable(Neutrino
	main.cpp
)

# Verlinke mit der NeutrinoEngine-Bibliothek
target_link_libraries(Neutrino
	PRIVATE NeutrinoEngine
)

# Header-Include-Verzeichnisse
target_include_directories(Neutrino
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}
		${CMAKE_SOURCE_DIR}/engine/include
)

# C++ Standard
target_compile_features(Neutrino PRIVATE cxx_std_20)

# Platform-spezifische Eigenschaften
if(WIN32)
	# Windows Console-Anwendung
	set_target_properties(Neutrino PROPERTIES
		SUFFIX ".exe"
	)
elseif(LINUX OR APPLE)
	# Linux/macOS Anwendung (keine Erweiterung erforderlich)
	set_target_properties(Neutrino PROPERTIES
		SUFFIX ""
	)
endif()

message(STATUS "Neutrino Executable konfiguriert")
