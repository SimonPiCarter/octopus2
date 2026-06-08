#pragma once

#include <string>
#include "flecs.h"
#include "octopus/utils/fast_map.hh"

namespace octopus {

/// @brief Allowed production for a player
struct PlayerProduction {
	std::fast_map<std::string, bool> productions;
};

bool satisfy_player_production_requirements(flecs::entity producer, std::string const &production_name);

} // namespace octopus
