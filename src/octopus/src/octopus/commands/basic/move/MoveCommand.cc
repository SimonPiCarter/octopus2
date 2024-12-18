#include "MoveCommand.hh"

#include "octopus/components/basic/flock/Flock.hh"

namespace octopus
{

bool move_routine(flecs::world &ecs, flecs::entity e, Position const&pos_p, Position const&target_p, Move &move_p, Flock const *flock_p)
{
	Fixed tol_l = Fixed::One()/10;
	if(flock_p)
	{
		uint32_t arrived_l = flock_p->arrived;
		tol_l += Fixed::One() + Fixed::One()*2*arrived_l;
	}

	if(square_length(pos_p.pos - target_p.pos) < tol_l)
	{
		return true;
	}
	// this will be updated
	move_p.move = get_speed_direction(ecs, pos_p, target_p, move_p.speed);
	// this is kept to know in which direction we wanted to move
	move_p.target_move = move_p.move;
	return false;
}

} // octopus
