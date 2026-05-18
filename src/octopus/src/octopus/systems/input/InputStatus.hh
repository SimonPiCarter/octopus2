#pragma once

#include <vector>
#include <string>

namespace octopus {

/// @brief This struct is used to return the status of an input command, it can be used to explain to the player why a command is not castable or producable
struct InputStatus {
	bool ok = true;
	flecs::entity entity;
	std::unordered_map<std::string, Fixed> resource_cost;
	std::vector<std::string> missing_upgrades;
	std::vector<std::string> other_explanations;
	double cooldown_ratio = 0.;
	uint64_t cooldown_ticks_remaining = 0;
};

} // octopus
