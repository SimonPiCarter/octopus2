#pragma once

#include "octopus/utils/ThreadPool.hh"
#include "octopus/world/position/PositionContext.hh"
#include "octopus/world/stats/TimeStats.hh"
#include "flecs.h"

namespace octopus
{

/// @brief Store all context required for standard operation
/// @note should contain the spatialisation and the pathfinding contexts for example
struct WorldContext
{
	WorldContext() : pool(1), position_context(ecs)
	{
		ecs.set_threads(12);
	}

	flecs::world ecs;
	ThreadPool pool;
	PositionContext position_context;
	TimeStats time_stats;

	/// @brief tell if the AttackSystems should wait for
	/// some time before looking for new target
	/// This is a the number of step between to
	/// target scan.
	/// @note increasing this number will speed up
	/// the engine but slow down reactiveness of units
	/// @note should be either 1 or powers of 2
	/// @note this is the number of step between to target scan.
	int64_t attack_retarget_wait = 1;
};

} // namespace octopus

