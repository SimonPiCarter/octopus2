#pragma once

#include <unordered_map>
#include <cstdint>

namespace octopus
{

struct PlayerInfo
{
	uint32_t idx = 0;
	uint32_t team = 0;
};

struct PlayerAppartenance
{
	uint32_t idx = 0;
};


} // namespace octopus
