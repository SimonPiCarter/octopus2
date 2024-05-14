# ============================================================================
# CMake configuration
# ============================================================================

cmake_minimum_required (VERSION 3.14.0)

# ============================================================================
# External requirements
# ============================================================================

find_package (GTest REQUIRED)

# ============================================================================
# Targets
# ============================================================================

# ----------------------------------------------------------------------------
# unit_tests
# ----------------------------------------------------------------------------

add_executable (unit_tests)


target_sources (unit_tests
	PRIVATE
	src/command_queue_chaining.test
	src/command_queue.test.cc
	src/comp_exclusion_bench.cc
	src/state_exclusion_bench.cc
	src/state_exclusive.test.cc
	src/state_extension.test.cc
	src/state.test.cc
)

target_link_libraries (unit_tests
	PRIVATE
	GTest::gtest_main
	octopus
)
