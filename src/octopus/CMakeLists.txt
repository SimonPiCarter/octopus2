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

add_library(octopus ${SOURCES})

target_link_libraries(octopus PUBLIC flecs::flecs_static Boost::boost)
target_include_directories(octopus
	PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/>
)

add_subdirectory(cdt)
add_subdirectory(test)

# ============================================================================
# Installation
# ============================================================================

install (TARGETS octopus EXPORT octopus-export)

install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/src/" # source directory
        DESTINATION "include/" # target directory
        FILES_MATCHING # install only matched files
        PATTERN "*.hh" # select header files
        PATTERN "*Graph.hh" EXCLUDE # exclude this deprecated file (exposing boost headers)
)

set (ConfigPackageLocation "lib/cmake/${PROJECT_NAME}-${PROJECT_VERSION}")
