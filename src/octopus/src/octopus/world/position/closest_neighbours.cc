#include "closest_neighbours.hh"

#include <set>

namespace octopus
{

std::vector<flecs::entity> get_closest_entities(
	size_t n,
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

	Fixed max_range_sq = max_range*max_range;

	// clear queued actions first
	context_p.position_query.each([&closest_l, &pos_p, &filter_p, &n, &max_range_sq](flecs::entity e, Position const &other_p) {

		Fixed distance_sq = square_length(pos_p.pos - other_p.pos);

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
	});

	std::vector<flecs::entity> result_l;
	for(auto &&dis_ent : closest_l)
	{
		result_l.push_back(dis_ent.e);
	}
	return result_l;
}

} // namespace octopus
