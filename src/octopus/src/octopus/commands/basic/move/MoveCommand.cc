#include "MoveCommand.hh"

namespace octopus
{

bool move_routine(flecs::world &ecs, flecs::entity e, Position const&pos_p, MoveCommand const &moveCommand_p, Move &move_p)
{
	if(square_length(pos_p.pos - moveCommand_p.target.pos) < Fixed::One()/100)
	{
		return true;
	}
	else
	{
		Vector direction_l = get_direction(ecs, pos_p, moveCommand_p.target);
		move_p.move = direction_l * move_p.speed;
	}
	return false;
}

} // octopus
