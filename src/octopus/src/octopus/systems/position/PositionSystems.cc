#include "PositionSystems.hh"

namespace octopus
{

Vector seek_force(Vector const &direction_p, Vector const &velocity_p, Fixed const &max_speed_p)
{
	Vector force;
	if(square_length(direction_p) > 0.01)
	{
		force = direction_p/length(direction_p) * max_speed_p;
		force = force - velocity_p;
	}
	return force;
}

Vector separation_force(flecs::iter& it, size_t i, flecs::field<const octopus::Position> const &pos_p)
{
	Vector force;
	Fixed max_range = Fixed(2);
	Fixed max_range_squared = max_range*max_range;
	Fixed force_factor = 100;

	for(size_t j = 0 ; j < it.count() ; ++ j)
	{
		if(j==i) {continue;}
		if(!pos_p[j].collision || pos_p[j].mass == Fixed::Zero())
		{
			continue;
		}
		Vector diff = pos_p[i].pos - pos_p[j].pos;
		Fixed length_squared = square_length(diff);
		if(length_squared <= max_range_squared && length_squared > 0.001)
		{
			Vector local_force = diff/length(diff) * force_factor / length_squared;
			// account for mass
			local_force *= 2 * pos_p[j].mass / (pos_p[i].mass + pos_p[j].mass);
			force += local_force;
		}
	}

	return force;
}

} // namespace octopus
