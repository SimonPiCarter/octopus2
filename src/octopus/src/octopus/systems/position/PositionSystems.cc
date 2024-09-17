#include "PositionSystems.hh"
#include "octopus/utils/aabb/aabb.hh"
#include <cassert>

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

	std::function<bool(int32_t, flecs::entity)> func_l = [&](int32_t idx_l, flecs::entity e) -> bool {
		Position const *pos_l = e.get<Position>();
		assert(pos_l);
		if(!pos_l->collision || pos_l->mass == Fixed::Zero())
		{
			return true;
		}
		Vector diff = pos_ref_p.pos - pos_l->pos;
		Fixed length_squared = square_length(diff);
		Fixed ray_squared = (pos_ref_p.ray + pos_l->ray)*(pos_ref_p.ray + pos_l->ray);
		Fixed max_range_squared = ray_squared * 2;
		if(length_squared <= ray_squared && length_squared > 0.001)
		{
			Vector local_force = diff/length(diff) * force_factor * 10 / length_squared;
			// account for mass
			local_force *= 2 * pos_l->mass / (pos_ref_p.mass + pos_l->mass);
			force += local_force;
		}
		else if(length_squared <= max_range_squared && length_squared > 0.001)
		{
			Vector local_force = diff/length(diff) * force_factor / length_squared;
			// account for mass
			local_force *= 2 * pos_l->mass / (pos_ref_p.mass + pos_l->mass);
			force += local_force;
		}
		return true;
	};

	tree_circle_query(posContext_p.tree, pos_ref_p.pos, pos_ref_p.ray*10., func_l);

	return force;
}

} // namespace octopus
