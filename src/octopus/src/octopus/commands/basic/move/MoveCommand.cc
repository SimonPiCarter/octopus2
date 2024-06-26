#include "MoveCommand.hh"

namespace octopus
{

bool move_routine(flecs::world &ecs, flecs::entity e, Position const&pos_p, Position const&target_p, Move &move_p)
{
	if(square_length(pos_p.pos - target_p.pos) < Fixed::One()/100)
	{
		return true;
	}
	move_p.move = get_speed_direction(ecs, pos_p, target_p, move_p.speed);
	return false;
}

} // octopus
