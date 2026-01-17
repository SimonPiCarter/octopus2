#include "AttackCommandSystem.hh"

namespace octopus
{

bool health_check_alive(flecs::entity const &other, bool ally)
{
	if(!other.try_get<HitPoint>()) {
		return false;
	}
	if(other.try_get<HitPoint>()->qty <= Fixed::Zero())
	{
		return false;
	}
	// If allies ignore full health entities
	if(ally && other.try_get<HitPointMax>() && other.try_get<HitPointMax>()->qty <= other.try_get<HitPoint>()->qty)
	{
		return false;
	}
	return true;
}

flecs::entity get_new_target(flecs::entity const &e, PositionContext const &context_p, Position const&pos_p, octopus::Fixed const &range_p, bool ally)
{
	Team const *team_l = e.try_get<Team>();
	// get enemy closest entities
	std::vector<flecs::entity> new_candidates_l;

	if (!team_l) {
		return flecs::entity();
	}

	bool can_use_trees_team = (!ally || context_p.trees_team_hp.size() == 2) && team_l->team < context_p.trees_team_hp.size();
	if(can_use_trees_team)
	{
		size_t tree_idx = ally ? context_p.trees_team_hp[(team_l->team+1) % 2] : context_p.trees_team_hp[team_l->team];
		new_candidates_l = get_closest_entities(1, tree_idx, range_p, context_p, pos_p, [&e, ally](flecs::entity const &other_p) -> bool {
			return other_p != e && health_check_alive(other_p, ally);
		});
	}
	else
	{
		new_candidates_l = get_closest_entities(1, 0, range_p, context_p, pos_p, [&e, team_l, ally](flecs::entity const &other_p) -> bool {
			if(other_p != e && other_p.try_get<Team>() && health_check_alive(other_p, ally))
			{
				bool is_same_team = team_l->team == other_p.try_get<Team>()->team;
				// if looking for ally we need the team to be the same
				return is_same_team == ally;
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
