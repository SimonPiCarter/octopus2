#include "closest_neighbours.hh"

#include <set>

namespace octopus
{

std::vector<flecs::entity> get_closest_entities(size_t n, flecs::world &ecs, Position const &pos_p, std::function<bool(flecs::entity const&)> const &func_p)
{
	flecs::query<Position> q = ecs.query<Position>();

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

	// clear queued actions first
	q.each([&closest_l, &pos_p, &func_p, &n](flecs::entity e, Position const &other_p) {

		Fixed distance_sq = square_length(pos_p.pos - other_p.pos);

		if(func_p(e)
		&& distance_sq < closest_l.rbegin()->distance_sq)
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
