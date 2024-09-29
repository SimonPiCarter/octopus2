#pragma once

#include <cstdint>
#include <unordered_map>
#include "octopus/utils/FixedPoint.hh"
#include "octopus/utils/fast_map/fast_map.hh"
#include "ResourceInfo.hh"

namespace octopus
{

struct ResourceStock
{
	ResourceStock() = default;
	ResourceStock(std::unordered_map<std::string, ResourceInfo> const &map_p) : resource(map_p) {}
	fast_map<std::string, ResourceInfo> resource;
};

struct ResourceStockMemento {
	Fixed quantity;
	std::string resource;
};

struct ResourceStockStep {
	Fixed delta;
	std::string resource;

	typedef ResourceStock Data;
	typedef ResourceStockMemento Memento;

	void apply_step(Data &d, Memento &memento) const;

	void revert_step(Data &d, Memento const &memento) const;
};

/// @brief return true if the resources map minus the locked resources
/// is bigger than the required resources
/// @param resources_p the stored resource of a player
/// @param locked_resources_p the resource not consumed yet but locked for something else
/// @param required_resources_p the required resources
bool check_resources(
	fast_map<std::string, ResourceInfo> const &resources_p,
	std::unordered_map<std::string, Fixed> const &locked_resources_p,
	std::unordered_map<std::string, Fixed> const &required_resources_p);

}
