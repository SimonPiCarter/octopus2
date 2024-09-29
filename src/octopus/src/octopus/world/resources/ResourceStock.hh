#pragma once

#include <cstdint>
#include <unordered_map>
#include "octopus/utils/FixedPoint.hh"
#include "ResourceInfo.hh"

namespace octopus
{

struct ResourceStock
{
	std::unordered_map<std::string, ResourceInfo> resource;
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

}
