#include "AttackCommand.hh"

namespace octopus
{

flecs::entity get_new_target(flecs::entity const &e, PositionContext const &context_p, Position const&pos_p, octopus::Fixed const &range_p)
{
	Team const *team_l = e.get<Team>();
	// get enemy closest entities
	std::vector<flecs::entity> new_candidates_l = get_closest_entities(1, range_p, context_p, pos_p, [team_l](flecs::entity const &other_p) -> bool {
		if(!team_l)
		{
			return true;
		}
		if(other_p.get<Team>() && other_p.get<HitPoint>() && other_p.get<HitPoint>()->qty > Fixed::Zero())
		{
			return team_l->team != other_p.get<Team>()->team;
		}
		return false;
	});
	if (new_candidates_l.empty())
	{
		return flecs::entity();
	}
	return new_candidates_l[0];
}

bool in_attack_range(Position const * target_pos_p, Position const&pos_p, Attack const&attack_p)
{
	Fixed range_ray = attack_p.range + pos_p.ray;
	if(target_pos_p)
	{
		range_ray += target_pos_p->ray;
	}
	return target_pos_p
		&& square_length(pos_p.pos - target_pos_p->pos) <= range_ray*range_ray;
}

bool has_reloaded(uint32_t time_p, Attack const&attack_p)
{
	return time_p >= attack_p.reload + attack_p.reload_time;
}

} // octopus
