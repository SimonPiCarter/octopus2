#include "direction.hh"

namespace octopus
{

Vector get_direction(flecs::world &ecs, Position const &pos_p, Position const &target_p)
{
	Vector dir_l = target_p.pos - pos_p.pos;
	return dir_l/length(dir_l);
}

}
