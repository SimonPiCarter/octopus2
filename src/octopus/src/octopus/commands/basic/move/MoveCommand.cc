#include "MoveCommand.hh"

namespace octopus
{

bool move_routine(flecs::world &ecs, flecs::entity e, Position const&pos_p, Position const&target_p, Move &move_p)
{
	if(square_length(pos_p.pos - target_p.pos) < Fixed::One()/100)
	{
		return true;
	}
	else
	{
		Vector direction_l = get_direction(ecs, pos_p, target_p);
		move_p.move = direction_l * move_p.speed;
	}
	return false;
}

} // octopus
