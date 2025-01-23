#pragma once

#include <unordered_map>
#include <cstdint>
#include "flecs.h"

namespace octopus
{

struct PlayerInfo
{
	uint32_t idx = 0;
	uint16_t team = 0;
};

struct PlayerAppartenance
{
	uint32_t idx = 0;
};

flecs::entity get_player_from_appartenance(flecs::entity e, flecs::world const &ecs);

} // namespace octopus
