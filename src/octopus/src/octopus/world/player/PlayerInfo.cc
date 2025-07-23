#include "PlayerInfo.hh"

namespace octopus
{

flecs::entity get_player_from_appartenance(flecs::entity e, flecs::world const &ecs)
{
	flecs::query<PlayerInfo> query_player = ecs.query<PlayerInfo>();

	return query_player.find([&e](PlayerInfo& p) {
		return e.try_get<PlayerAppartenance>() && p.idx == e.try_get<PlayerAppartenance>()->idx;
	});
}

} // namespace octopus
