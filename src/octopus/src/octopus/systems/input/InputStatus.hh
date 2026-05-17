#pragma once

#include <vector>
#include <string>

namespace octopus {

/// @brief This struct is used to return the status of an input command, it can be used to explain to the player why a command is not castable or producable
struct InputStatus {
	bool ok = true;
	std::vector<std::string> missing_resources_player;
	std::vector<std::string> missing_resources_entity;
	std::vector<std::string> missing_upgrades;
	std::vector<std::string> other_explanations;
	double cooldown_ratio = 0.;
	uint64_t cooldown_ticks_remaining = 0;
};

} // octopus
