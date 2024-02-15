# ============================================================================
# CMake configuration
# ============================================================================

cmake_minimum_required (VERSION 3.14.0)

# ============================================================================
# Game Engine configuration
# ============================================================================

# Automatically pick up files added to src
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/octopus/*.hh"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/octopus/*.cc")

add_executable(octopus ${SOURCES})

target_link_libraries(octopus PRIVATE flecs::flecs_static)
target_include_directories(octopus
	PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/>
)

# ============================================================================
# Installation
# ============================================================================

if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
	set (CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install/${PROJECT_NAME}-${PROJECT_VERSION}" CACHE PATH "${PROJECT_NAME} default install path" FORCE)
endif ()

install (TARGETS octopus EXPORT octopus-export)

install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/src/" # source directory
        DESTINATION "include/" # target directory
        FILES_MATCHING # install only matched files
        PATTERN "*.hh" # select header files
		PATTERN "*Graph.hh" EXCLUDE # exclude this deprecated file (exposing boost headers)
)

set (ConfigPackageLocation "lib/cmake/${PROJECT_NAME}-${PROJECT_VERSION}")
