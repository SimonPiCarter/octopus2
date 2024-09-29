#pragma once

#include <cstdint>
#include <unordered_map>
#include "octopus/utils/FixedPoint.hh"

namespace octopus
{

struct ResourceInfo
{
	Fixed quantity;
	Fixed cap;
};

} // namespace octopus
