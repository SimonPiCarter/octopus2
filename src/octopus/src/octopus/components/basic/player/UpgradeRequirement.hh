#pragma once

#include "octopus/utils/fast_map/fast_map.hh"
#include <string>
#include <cstdint>

namespace octopus
{
struct PlayerUpgrade;

struct UpgradeRequirement
{
	fast_map<std::string, int64_t> upgrades;
};

bool check_requirements(UpgradeRequirement const &req, PlayerUpgrade const &up);
bool check_requirements(flecs::entity entity, flecs::world const &ecs, UpgradeRequirement const &requirements);

}
