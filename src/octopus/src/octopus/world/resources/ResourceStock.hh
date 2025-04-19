#pragma once

#include <cstdint>
#include <unordered_map>
#include "octopus/utils/FixedPoint.hh"
#include "octopus/utils/fast_map/fast_map.hh"
#include "ResourceInfo.hh"
#include "ResourceSpent.hh"

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

/// @brief This is the way to consume resource
/// as it will fill both ResourceSpent structure
/// and step manager.
/// It is important to do both so that further
/// resource check are coherent.
/// @param spent ResourceSpent component (if null nothing is done on it)
/// @param amount amount spent (negative if refound of resource)
template<typename StepManager>
void spend_resources(StepManager &manager, ResourceSpent *spent, flecs::entity e, Fixed const &amount, std::string const &resource)
{
	manager.get_last_layer().back().template get<ResourceStockStep>().add_step(e, {-amount, resource});
	if(spent)
	{
		spent->resources_spent[resource] += amount;
	}
}

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
