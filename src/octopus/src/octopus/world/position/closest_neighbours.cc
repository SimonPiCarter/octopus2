#include "closest_neighbours.hh"

#include <cassert>
#include <set>
#include "octopus/utils/aabb/aabb.hh"

namespace octopus
{

std::vector<flecs::entity> get_closest_entities(
	size_t n,
	size_t tree_idx,
	Fixed const &max_range,
	PositionContext const &context_p,
	Position const &pos_p,
	std::function<bool(flecs::entity const&)> const &filter_p)
{
	struct DistanceEntity {
		flecs::entity e;
		Fixed distance_sq;

		bool operator<(DistanceEntity const &other) const
		{
			if(distance_sq != other.distance_sq)
			{
				return distance_sq < other.distance_sq;
			}
			return e == other.e;
		}
	};

	std::set<DistanceEntity> closest_l;


	std::function<bool(int32_t, flecs::entity)> func_l = [&](int32_t idx_l, flecs::entity e) -> bool {
		Position const *other_p = e.get<Position>();
		assert(other_p);
		Fixed distance_sq = square_length(pos_p.pos - other_p->pos);
		Fixed max_range_sq = (max_range+other_p->ray)*(max_range+other_p->ray);

		if(filter_p(e)
		&& (closest_l.empty() || distance_sq < closest_l.rbegin()->distance_sq)
		&& distance_sq <= max_range_sq)
		{
			closest_l.insert({e, distance_sq});
		}

		if(closest_l.size() > n)
		{
			closest_l.erase(std::prev(closest_l.end()));
		}
		return true;
	};

	tree_circle_query(context_p.trees[tree_idx], pos_p.pos, max_range, func_l);

	std::vector<flecs::entity> result_l;
	for(auto &&dis_ent : closest_l)
	{
		result_l.push_back(dis_ent.e);
	}
	return result_l;
}

} // namespace octopus
