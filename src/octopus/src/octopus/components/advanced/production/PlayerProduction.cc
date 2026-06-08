#include "PlayerProduction.hh"

#include "octopus/world/player/PlayerInfo.hh"

namespace octopus {

void PlayerProductionStep::apply_step(Data &d, Memento &memento) const {
	memento.production_name = production_name;
	memento.old_value = d.productions[production_name];
	d.productions[production_name] = new_value;
}

void PlayerProductionStep::revert_step(Data &d, Memento const &memento) const {
	d.productions[memento.production_name] = memento.old_value;
}

bool satisfy_player_production_requirements(flecs::entity producer, std::string const &production_name) {
	const auto player = get_player_from_appartenance(producer, producer.world());
	// If the producer is not associated to a player or if the player doesn't have PlayerProduction, we consider that there is no requirement and return true
	if (!player.is_valid() || !player.has<PlayerProduction>()) {
		return true;
	}
	const auto &player_prod = player.get<PlayerProduction>();
	return player_prod.productions.safe_get(production_name, false);
}

} // namespace octopus
