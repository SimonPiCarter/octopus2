#pragma once

#include "flecs.h"
#include "octopus/utils/ThreadPool.hh"

#include "octopus/commands/queue/CommandQueue.hh"
#include "octopus/systems/hitpoint/HitPointsSystems.hh"
#include "octopus/systems/step/StepSystems.hh"

namespace octopus
{
/// @brief Set up all required system for the engine to run
/// @param ecs
template<typename variant_t>
void set_up_systems(flecs::world &ecs, ThreadPool &pool, CommandQueueMementoManager<variant_t> &memento_manager, StepManager &step_manager)
{
	// command handling systems
	set_up_command_queue_systems<variant_t>(ecs, memento_manager);

	// step systems
	set_up_step_systems(ecs, pool, step_manager);

	// all validators
	set_up_hitpoint_systems(ecs, pool);
}

}
