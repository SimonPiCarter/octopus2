#include "PlayerInfo.hh"

namespace octopus
{

flecs::entity get_player_from_appartenance(flecs::entity e, flecs::world const &ecs)
{
	flecs::query<PlayerInfo> query_player = ecs.query<PlayerInfo>();

	return query_player.find([&e](PlayerInfo& p) {
		return e.get<PlayerAppartenance>() && p.idx == e.get<PlayerAppartenance>()->idx;
	});
}

} // namespace octopus
