#pragma once

namespace octopus
{

struct PathQuery
{
	bool is_valid() const { return false; }
	Vector get_direction() const { return {}; }
};

struct PathFindingCache
{
	PathQuery queryPath(flecs::world &ecs, Position const &pos, Vector const &target) const { return PathQuery(); }
};

}
