#pragma once

#include "flecs.h"
#include "octopus/utils/ThreadPool.hh"

#include "octopus/commands/basic/ability/CastCommand.hh"
#include "octopus/commands/queue/CommandQueue.hh"
#include "octopus/commands/basic/move/AttackCommand.hh"
#include "octopus/commands/basic/move/MoveCommand.hh"
#include "octopus/systems/hitpoint/HitPointsSystems.hh"
#include "octopus/systems/position/PositionSystems.hh"
#include "octopus/systems/step/StepSystems.hh"
#include "octopus/systems/production/ProductionSystem.hh"
#include "octopus/systems/input/Input.hh"

#include "octopus/world/production/ProductionTemplateLibrary.hh"
#include "octopus/world/StepContext.hh"
#include "octopus/world/WorldContext.hh"

#include "octopus/systems/phases/Phases.hh"

namespace octopus
{

void set_up_phases(flecs::world &ecs);

/// @brief Set up all required system for the engine to run
template<typename StepContext_t>
void set_up_systems(
	WorldContext &world,
	StepContext_t &step_context,
	ProductionTemplateLibrary<typename StepContext_t::step> *production_library = nullptr,
	AbilityTemplateLibrary<typename StepContext_t::step> *ability_library = nullptr,
	uint32_t step_kept_p = 0
)
{
	set_up_phases(world.ecs);

	// command handling systems
	set_up_command_queue_systems<typename StepContext_t::variant>(world.ecs, step_context.memento_manager, step_context.state_step_manager, step_kept_p);

	// position systems
	set_up_position_systems(world.ecs, world.pool, step_context.step_manager, world.position_context, world.time_stats);

	// step systems
	set_up_step_systems(world.ecs, world.pool, step_context.step_manager, step_context.state_step_manager);

	// components systems
	set_up_hitpoint_systems(world.ecs, world.pool, step_context.step_manager, step_kept_p);

	// commands systems
	set_up_move_system<typename StepContext_t::step, CommandQueue<typename StepContext_t::variant>>(world.ecs, step_context.step_manager, world.time_stats);
	set_up_attack_system<typename StepContext_t::step, CommandQueue<typename StepContext_t::variant>>(
		world.ecs, step_context.step_manager, world.position_context, world.time_stats, world.attack_retarget_wait);

	if(ability_library != nullptr)
	{
		set_up_cast_system<typename StepContext_t::step, CommandQueue<typename StepContext_t::variant>>(
			world.ecs, step_context.step_manager, *ability_library
		);
	}

	// production systems
	if(production_library != nullptr)
	{
		set_up_production_systems(world.ecs, world.pool, step_context.step_manager, *production_library, world.time_stats);
	}

	// input systems
	set_up_input_system<typename StepContext_t::variant, typename StepContext_t::step>(world.ecs, production_library, step_context.step_manager);
}

}
