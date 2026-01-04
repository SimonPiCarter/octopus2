#include "AttackCommandSystem.hh"

namespace octopus
{

flecs::entity get_new_target(flecs::entity const &e, PositionContext const &context_p, Position const&pos_p, octopus::Fixed const &range_p)
{
	Team const *team_l = e.try_get<Team>();
	// get enemy closest entities
	std::vector<flecs::entity> new_candidates_l;
	if(team_l && team_l->team < context_p.trees_team_hp.size())
	{
		new_candidates_l = get_closest_entities(1, context_p.trees_team_hp[team_l->team], range_p, context_p, pos_p, [](flecs::entity const &other_p) -> bool {
			if(other_p.try_get<HitPoint>() && other_p.try_get<HitPoint>()->qty > Fixed::Zero())
			{
				return true;
			}
			return false;
		});
	}
	// if team
	else if(team_l)
	{
		new_candidates_l = get_closest_entities(1, 0, range_p, context_p, pos_p, [team_l](flecs::entity const &other_p) -> bool {
			if(other_p.try_get<Team>() && other_p.try_get<HitPoint>() && other_p.try_get<HitPoint>()->qty > Fixed::Zero())
			{
				return team_l->team != other_p.try_get<Team>()->team;
			}
			return false;
		});
	}

	if (new_candidates_l.empty())
	{
		return flecs::entity();
	}
	return new_candidates_l[0];
}

bool in_attack_range(Position const * target_pos_p, Collision const * target_col_p, Position const&pos_p, Collision const&col_p, Attack const&attack_p)
{
	Fixed range_ray = attack_p.cst.range + col_p.ray;
	if(target_pos_p)
	{
		range_ray += target_col_p->ray;
	}
	return target_pos_p
		&& square_length(pos_p.pos - target_pos_p->pos) <= range_ray*range_ray;
}

bool has_reloaded(uint32_t time_p, Attack const&attack_p)
{
	return int32_t(time_p) >= attack_p.reload + attack_p.cst.reload_time;
}

} // octopus
