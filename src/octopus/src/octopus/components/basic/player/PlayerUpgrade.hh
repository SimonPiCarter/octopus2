#pragma once

#include "octopus/utils/fast_map/fast_map.hh"
#include <string>
#include <cstdint>

namespace octopus
{

struct PlayerUpgrade
{
	fast_map<std::string, int64_t> upgrades;
};

struct PlayerUpgradeMemento {
	int64_t value = 0;
};

struct PlayerUpgradeStep {
	std::string upgrade;
	int64_t delta = 1;

	typedef PlayerUpgrade Data;
	typedef PlayerUpgradeMemento Memento;

	void apply_step(Data &d, Memento &memento) const
	{
		memento.value = d.upgrades[upgrade];
		d.upgrades[upgrade] += delta;
	}

	void revert_step(Data &d, Memento const &memento) const
	{
		d.upgrades[upgrade] = memento.value;
	}
};

bool check_upgrades(PlayerUpgrade const &upgrades, std::string const &upgrade, int64_t level);

} // namespace octopus
