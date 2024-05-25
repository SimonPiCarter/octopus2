#pragma once

#include "octopus/components/basic/position/Position.hh"
#include "octopus/components/basic/position/Move.hh"
#include "flecs.h"

namespace octopus
{

struct PositionContext
{
	PositionContext(flecs::world &ecs) : position_query(ecs.query<Position const>()), move_query(ecs.query<Position const, Move>()) {}

	flecs::query<Position const> const position_query;
	flecs::query<Position const, Move> const move_query;
};

} // namespace octopus
