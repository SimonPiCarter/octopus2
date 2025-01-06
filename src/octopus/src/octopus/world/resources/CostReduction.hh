#pragma once

#include <cstdint>
#include <unordered_map>
#include "octopus/utils/FixedPoint.hh"
#include "octopus/utils/fast_map/fast_map.hh"
#include "ResourceInfo.hh"

namespace octopus
{

struct CostReduction
{
	CostReduction() = default;
	CostReduction(std::unordered_map<std::string, Fixed> const &map_p) : reduction(map_p) {}
	fast_map<std::string, Fixed> reduction;
};

struct ReductionLibrary
{
	fast_map<std::string, CostReduction> reductions;
};

struct ReductionLibraryMemento {
	Fixed quantity;
	std::string resource;
	std::string production;
};

struct ReductionLibraryStep {
	Fixed delta;
	std::string resource;
	std::string production;

	typedef ReductionLibrary Data;
	typedef ReductionLibraryMemento Memento;

	void apply_step(Data &d, Memento &memento) const;

	void revert_step(Data &d, Memento const &memento) const;
};

/// @brief return the required resourced based on the cost reduction
std::unordered_map<std::string, Fixed> get_required_resources(
	CostReduction const &cost_reduction,
	std::unordered_map<std::string, Fixed> const &required_resources);

}
