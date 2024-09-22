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

/// @brief return true if the resources map minus the locked resources
/// is bigger than the required resources
/// @param resources_p the stored resource of a player
/// @param locked_resources_p the resource not consumed yet but locked for something else
/// @param required_resources_p the required resources
bool check_resources(
	std::unordered_map<std::string, ResourceInfo> const &resources_p,
	std::unordered_map<std::string, Fixed> const &locked_resources_p,
	std::unordered_map<std::string, Fixed> const &required_resources_p);

} // namespace octopus
