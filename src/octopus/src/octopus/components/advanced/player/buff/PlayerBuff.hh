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

} // namespace octopus
