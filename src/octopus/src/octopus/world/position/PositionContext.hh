#pragma once

#include <array>
#include <functional>

#include "octopus/components/basic/position/Position.hh"
#include "octopus/components/basic/position/Move.hh"
#include "octopus/components/basic/player/Team.hh"
#include "octopus/components/basic/hitpoint/HitPoint.hh"
#include "flecs.h"

#include "octopus/utils/aabb/aabb_tree.hh"

namespace octopus
{

template<uint16_t team_idx>
bool check_team(flecs::entity e)
{
	Team const *team = e.get<Team>();
	return team && team->team != team_idx;
}

struct PositionContext
{
	PositionContext(flecs::world &ecs) : position_query(ecs.query<Position const>()), move_query(ecs.query<Position const, Move>())
	{
		static_assert( std::tuple_size<decltype(tree_filters)>::value == std::tuple_size<decltype(trees)>::value );

		tree_filters[0] = [] (flecs::entity e) -> bool { return true; };

		// trees_team_hp for quick access
		tree_filters[1] = [] (flecs::entity e) -> bool { return check_team<0>(e) && e.get<HitPoint>(); };
		tree_filters[2] = [] (flecs::entity e) -> bool { return check_team<1>(e) && e.get<HitPoint>(); };
		trees_team_hp[0] = 1;
		trees_team_hp[1] = 2;
	}

	flecs::query<Position const> const position_query;
	flecs::query<Position const, Move> const move_query;

	// Multiple trees with different entities to make query faster
	// depending on the filters
	// - 0 : all
	// Only entities of other teams given by the index with hp
	// - 1 : all other teams than 0 with HP
	// - 2 : all other teams than 1 with HP
	std::array<aabb_tree<flecs::entity>, 3> trees;
	std::array<std::function<bool(flecs::entity)>, 3> tree_filters;

	// index to trees
	std::array<size_t, 2> trees_team_hp;
};

} // namespace octopus
