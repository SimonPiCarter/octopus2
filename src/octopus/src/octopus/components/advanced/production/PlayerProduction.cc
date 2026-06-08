#include "PlayerProduction.hh"

#include "octopus/world/player/PlayerInfo.hh"

namespace octopus {

bool satisfy_player_production_requirements(flecs::entity producer, std::string const &production_name) {
	const auto player = get_player_from_appartenance(producer, producer.world());
	// If the producer is not associated to a player or if the player doesn't have PlayerProduction, we consider that there is no requirement and return true
	if (!player.is_valid() || !player.has<PlayerProduction>()) {
		return true;
	}
	const auto &player_prod = player.get<PlayerProduction>();
	return player_prod->productions.safe_get(production_name, false);
}

} // namespace octopus
