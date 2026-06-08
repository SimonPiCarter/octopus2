#pragma once

#include <string>
#include "flecs.h"
#include "octopus/utils/fast_map/fast_map.hh"

namespace octopus {

/// @brief Allowed production for a player
struct PlayerProduction {
	octopus::fast_map<std::string, bool> productions;
};

struct PlayerProductionMemento {
    std::string production_name;
	bool old_value = false;
};

struct PlayerProductionStep {
    std::string production_name;
	bool new_value = false;

	typedef PlayerProduction Data;
	typedef PlayerProductionMemento Memento;

	void apply_step(Data &d, Memento &memento) const;

	void revert_step(Data &d, Memento const &memento) const;
};

bool satisfy_player_production_requirements(flecs::entity producer, std::string const &production_name);

} // namespace octopus
