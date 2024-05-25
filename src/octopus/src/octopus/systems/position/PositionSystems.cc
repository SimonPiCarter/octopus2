#include "PositionSystems.hh"

namespace octopus
{

Fixed get_coef(Fixed const &dist_sq)
{
	/// Affin coefficient
	static const Fixed min_ = 0.5*.05;
	static const Fixed max_ = 2*2;
	static const Fixed min_val_ = 4;
	static const Fixed max_val_ = 8;
	static const Fixed a_ = (min_val_ - max_val_) / (max_ - min_);
	static const Fixed b_ = max_val_ - min_ * a_;

	if(dist_sq < max_)
	{
		return std::max(Fixed(0.1), a_ * dist_sq * b_);
	}
	return 0;
}

Vector get_separation(flecs::entity e, Position const &pos_p, PositionContext const &posContext_p, Move const &move_p)
{
	Vector steer;
	int c = 0;
	Fixed max_coef;
	posContext_p.position_query.each([&](flecs::entity oe, Position const &other_p)
	{
		Fixed d = square_length(pos_p.pos-other_p.pos);
		Fixed coef = get_coef(d);
		if (e != oe && coef > 0.0001)
		{
			max_coef = std::max(max_coef, coef);
			Vector direction = pos_p.pos-other_p.pos;
			Fixed len = 1;
			if(d > 0.0001)
			{
				len = length(direction);
			}
			else
			{
				direction = e > oe ? Vector(1,0) : Vector(-1,0);
			}
			direction /= len;
			direction *= coef;
			// direction /= len;

			steer += direction;
			++c;
		}
	});

	if(c > 0)
	{
		steer /= c;
	}

	if(square_length(steer) > 0.0001)
	{
		steer = steer/length(steer) * move_p.speed - move_p.move;
		steer *= max_coef;
	}

	return steer;
}

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

Vector separation_force(flecs::iter& it, size_t i, Position const *pos_p)
{
	Vector force;
	Fixed max_range = Fixed(2);
	Fixed max_range_squared = max_range*max_range;
	Fixed force_factor = 100;

	for(size_t j = 0 ; j < it.count() ; ++ j)
	{
		if(j==i) {continue;}
		Vector diff = pos_p[i].pos - pos_p[j].pos;
		Fixed length_squared = square_length(diff);
		if(length_squared <= max_range_squared && length_squared > 0.001)
		{
			force += diff/length(diff) * force_factor / length_squared;
		}
	}

	return force;
}

} // namespace octopus
