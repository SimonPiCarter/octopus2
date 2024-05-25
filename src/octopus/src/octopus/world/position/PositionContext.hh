#pragma once

#include "octopus/components/basic/position/Position.hh"
#include "flecs.h"

namespace octopus
{

struct PositionContext
{
	PositionContext(flecs::world &ecs) : position_query(ecs.query<Position>()) {}

	flecs::query<Position> const position_query;
};

} // namespace octopus
