#pragma once

#include "flecs.h"
#include "octopus/utils/ThreadPool.hh"

#include "octopus/commands/queue/CommandQueue.hh"
#include "octopus/systems/hitpoint/HitPointsSystems.hh"

namespace octopus
{
/// @brief Set up all required system for the engine to run
/// @param ecs
template<typename variant_t>
void set_up_systems(flecs::world &ecs, ThreadPool &pool)
{
	// command handling systems
	set_up_command_queue_systems<variant_t>(ecs);

	// all validators
	set_up_hitpoint_systems(ecs, pool);
}

}
