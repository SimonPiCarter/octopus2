#pragma once

#include "octopus/utils/ThreadPool.hh"
#include "octopus/world/position/PositionContext.hh"
#include "flecs.h"

namespace octopus
{

/// @brief Store all context required for standard operation
/// @note should contain the spatialisation and the pathfinding contexts for example
struct WorldContext
{
	WorldContext() : pool(1), position_context(ecs) {}

	flecs::world ecs;
	ThreadPool pool;
	PositionContext position_context;
};

} // namespace octopus

