#include "direction.hh"

namespace octopus
{

Vector get_direction(flecs::world &ecs, Position const &pos_p, Position const &target_p)
{
	Vector dir_l = target_p.pos - pos_p.pos;
	return dir_l/length(dir_l);
}

Vector get_speed_direction(flecs::world &ecs, Position const &pos_p, Position const &target_p, Fixed const &speed_p)
{
	Vector dir_l = target_p.pos - pos_p.pos;
	if(square_length(dir_l) > speed_p*speed_p)
	{
		dir_l = dir_l/length(dir_l)*speed_p;
	}
	return dir_l;
}

}
