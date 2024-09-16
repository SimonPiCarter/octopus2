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

Vector separation_force(PositionContext const &posContext_p, Position const &pos_ref_p)
{
	Vector force;
	Fixed force_factor = 500;

	posContext_p.move_query.run([&](flecs::iter& it) {
		while(it.next()) {
		auto pos_l = it.field<Position const>(0);
		for(size_t j = 0 ; j < it.count() ; ++ j)
		{
			if(!pos_l[j].collision || pos_l[j].mass == Fixed::Zero())
			{
				continue;
			}
			Vector diff = pos_ref_p.pos - pos_l[j].pos;
			Fixed length_squared = square_length(diff);
			Fixed ray_squared = (pos_ref_p.ray + pos_l[j].ray)*(pos_ref_p.ray + pos_l[j].ray);
			Fixed max_range_squared = ray_squared * 2;
			if(length_squared <= ray_squared && length_squared > 0.001)
			{
				Vector local_force = diff/length(diff) * force_factor * 10 / length_squared;
				// account for mass
				local_force *= 2 * pos_l[j].mass / (pos_ref_p.mass + pos_l[j].mass);
				force += local_force;
			}
			else if(length_squared <= max_range_squared && length_squared > 0.001)
			{
				Vector local_force = diff/length(diff) * force_factor / length_squared;
				// account for mass
				local_force *= 2 * pos_l[j].mass / (pos_ref_p.mass + pos_l[j].mass);
				force += local_force;
			}
		}}
	});

	return force;
}

} // namespace octopus
