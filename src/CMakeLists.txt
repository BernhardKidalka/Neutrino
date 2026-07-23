cmake_minimum_required(VERSION 3.20)

# ==============================================================================
# Neutrino Executable
# ==============================================================================

add_executable(Neutrino
	main.cpp
)

# Link NeutrinoEngine library
target_link_libraries(Neutrino
	PRIVATE NeutrinoEngine
	PRIVATE glfw
)

# Header include directories
target_include_directories(Neutrino
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}
		${CMAKE_SOURCE_DIR}/engine/include
)

# C++ Standard
target_compile_features(Neutrino PRIVATE cxx_std_20)

# Platform-specific properties
if(WIN32)
	# Windows console application
	set_target_properties(Neutrino PROPERTIES
		SUFFIX ".exe"
	)
elseif(LINUX OR APPLE)
	# Linux/macOS application (no extension required)
	set_target_properties(Neutrino PROPERTIES
		SUFFIX ""
	)
endif()

message(STATUS "Neutrino Executable configured")
