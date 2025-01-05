# ============================================================================
# CMake configuration
# ============================================================================

cmake_minimum_required(VERSION 3.19)

# ============================================================================
# Project configuration
# ============================================================================

project (octopus
	VERSION 1.0.0
	LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# ===========================================================
# External requirements
# ===========================================================

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

# ----------------------------------------------------------------------------
# Optional conan setup
# ----------------------------------------------------------------------------
if (EXISTS ${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
	include (${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
	conan_basic_setup (#[[TARGETS]])
endif ()

# ----------------------------------------------------------------------------
# External requirements setup
# ----------------------------------------------------------------------------
# additional cmake modules
list (PREPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
list (PREPEND CMAKE_PREFIX_PATH "${CMAKE_BINARY_DIR}")

# library behaviour
set (CMAKE_FIND_PACKAGE_SORT_ORDER NATURAL)
set (CMAKE_FIND_PACKAGE_SORT_DIRECTION DEC)
set (CMAKE_FIND_PACKAGE_PREFER_CONFIG ON)

find_package(Boost 1.83.0 REQUIRED)
add_definitions(-DBOOST_ALLOW_DEPRECATED_HEADERS)

# ============================================================================
# Game Engine configuration
# ============================================================================

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_definitions(-D_NOT_GODOT_BUILD_)

# add_subdirectory(src/flecs)
add_subdirectory(src/octopus)
add_subdirectory(src/exe)

# ============================================================================
# Installation
# ============================================================================

set (ConfigPackageLocation "lib/cmake/${PROJECT_NAME}-${PROJECT_VERSION}")

# ----------------------------------------------------------------------------
# Configuration files
# ----------------------------------------------------------------------------
write_basic_package_version_file ("${CMAKE_CURRENT_BINARY_DIR}/cmake/${PROJECT_NAME}-config-version.cmake"
	VERSION ${PROJECT_VERSION}
	COMPATIBILITY SameMajorVersion)

configure_package_config_file (cmake/${PROJECT_NAME}-config.cmake.in
	"${CMAKE_CURRENT_BINARY_DIR}/cmake/${PROJECT_NAME}-config.cmake"
	INSTALL_DESTINATION "${ConfigPackageLocation}"
	NO_SET_AND_CHECK_MACRO
	NO_CHECK_REQUIRED_COMPONENTS_MACRO)

# ----------------------------------------------------------------------------
# Export
# ----------------------------------------------------------------------------

set (CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install/${PROJECT_NAME}-${PROJECT_VERSION}" CACHE PATH "${PROJECT_NAME} default install path" FORCE)

install (FILES
	"${CMAKE_CURRENT_BINARY_DIR}/cmake/${PROJECT_NAME}-config.cmake"
	"${CMAKE_CURRENT_BINARY_DIR}/cmake/${PROJECT_NAME}-config-version.cmake"
	DESTINATION "${ConfigPackageLocation}"
)

install (EXPORT ${PROJECT_NAME}-export
	FILE ${PROJECT_NAME}-targets.cmake
	NAMESPACE ${PROJECT_NAME}::
	DESTINATION "${ConfigPackageLocation}"
	)
