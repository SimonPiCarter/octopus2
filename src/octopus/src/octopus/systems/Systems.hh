#pragma once

#include "flecs.h"
#include "octopus/utils/ThreadPool.hh"

#include "octopus/commands/queue/CommandQueue.hh"
#include "octopus/commands/basic/move/MoveCommand.hh"
#include "octopus/systems/hitpoint/HitPointsSystems.hh"
#include "octopus/systems/position/PositionSystems.hh"
#include "octopus/systems/step/StepSystems.hh"

#include "octopus/systems/phases/Phases.hh"

namespace octopus
{

void set_up_phases(flecs::world &ecs);

/// @brief Set up all required system for the engine to run
/// @param ecs
template<typename variant_t, typename StepManager_t>
void set_up_systems(flecs::world &ecs, ThreadPool &pool, CommandQueueMementoManager<variant_t> &memento_manager, StepManager_t &step_manager)
{
	set_up_phases(ecs);

	// command handling systems
	set_up_command_queue_systems<variant_t>(ecs, memento_manager);

	// position systems
	set_up_position_systems(ecs, pool, step_manager);

	// step systems
	set_up_step_systems(ecs, pool, step_manager);

	// components systems
	set_up_hitpoint_systems(ecs, pool);

	// commands systems
	set_up_move_system<StepManager_t, CommandQueue<variant_t>>(ecs, step_manager);
}

}
