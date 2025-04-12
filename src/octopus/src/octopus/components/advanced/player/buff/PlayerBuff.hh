#pragma once

#include <string>
#include <vector>

namespace octopus
{

template<typename TargetType, typename BuffType, typename... ComponentType>
struct PlayerBuff
{
    BuffType buff;
};

/// @brief This is an event to be used before saving since all buffs will be reapplied on load
struct DebuffAll
{};

} // namespace octopus
