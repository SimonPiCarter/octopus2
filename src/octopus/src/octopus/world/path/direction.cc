#include "direction.hh"
#include "PathFindingCache.hh"

namespace octopus
{

Vector get_direction(flecs::world &ecs, Position const &pos_p, Vector const &target_p)
{
	Vector dir_l = target_p - pos_p.pos;
	return dir_l/length(dir_l);
}

Vector get_speed_direction(flecs::world &ecs, Position const &pos_p, Vector const &target_p, Fixed const &speed_p)
{
	// Query path finding
	PathFindingCache const * cache = ecs.get<PathFindingCache>();
	// if not found -> direct path
	Vector dir_l = target_p - pos_p.pos;
	// if found -> use 'quick' funnel
	if(cache)
	{
		PathQuery query = cache->queryPath(ecs, pos_p, target_p);
		if(query.is_valid())
		{
			dir_l = query.get_direction();
		}
	}
	if(square_length(dir_l) > speed_p*speed_p)
	{
		dir_l = dir_l/length(dir_l)*speed_p;
	}
	return dir_l;
}

}
