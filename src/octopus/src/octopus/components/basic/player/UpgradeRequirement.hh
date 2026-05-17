#pragma once

#include "octopus/utils/fast_map/fast_map.hh"
#include <string>
#include <cstdint>
#include <vector>

namespace octopus
{
struct PlayerUpgrade;

struct UpgradeRequirement
{
	fast_map<std::string, int64_t> upgrades;
};

bool check_requirements(UpgradeRequirement const &req, PlayerUpgrade const &up);
bool check_requirements(flecs::entity entity, flecs::world const &ecs, UpgradeRequirement const &requirements);
std::vector<std::string> explain_unmet_requirements(flecs::entity entity, flecs::world const &ecs, UpgradeRequirement const &requirements);

}
