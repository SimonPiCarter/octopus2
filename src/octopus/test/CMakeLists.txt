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
	src/env/setup.test.cc
	src/serialization/ser_basic_components.test.cc
	src/serialization/ser_command_queue.test.cc
	src/serialization/ser_list.test.cc
	src/serialization/ser_variant.test.cc
	src/serialization/ser_vector.test.cc
	src/step/command/state_change_steps.test.cc
	src/step/components/basic/attack_loop.test.cc
	src/step/components/basic/attack_move_loop.test.cc
	src/step/components/basic/attack_retarget_loop.test.cc
	src/step/components/basic/hitpoint_loop.test.cc
	src/step/components/basic/move_loop.test.cc
	src/step/components/extended/extented_component_loop.test.cc
	src/command_queue_chaining.test.cc
	src/command_queue.test.cc
	src/comp_exclusion_bench.cc
	src/sandbox.test.cc
	src/state_exclusion_bench.cc
	src/state_exclusive.test.cc
	src/state_extension.test.cc
	src/state.test.cc
	src/step_container_hitpoint.test.cc
)

target_link_libraries (unit_tests
	PRIVATE
	GTest::gtest
	octopus
)

target_include_directories(unit_tests PRIVATE src)
