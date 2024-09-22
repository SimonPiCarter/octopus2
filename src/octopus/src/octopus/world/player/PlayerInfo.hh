#pragma once

#include <unordered_map>
#include <cstdint>
#include "ResourceInfo.hh"

namespace octopus
{

struct PlayerInfo
{
	uint32_t idx = 0;
	uint32_t team = 0;
	std::unordered_map<std::string, ResourceInfo> resource;
};

struct PlayerAppartenance
{
	uint32_t idx = 0;
};

struct ResourceInfoQuantityMemento {
	Fixed quantity;
	std::string resource;
};

struct ResourceInfoQuantityStep {
	Fixed delta;
	std::string resource;

	typedef PlayerInfo Data;
	typedef ResourceInfoQuantityMemento Memento;

	void apply_step(Data &d, Memento &memento) const;

	void revert_step(Data &d, Memento const &memento) const;
};


} // namespace octopus
