
#pragma once

#include "octopus/components/step/Step.hh"
#include "octopus/utils/FixedPoint.hh"

namespace octopus
{

struct Armor {
	Fixed qty;
};

struct ArmorMemento {
	Fixed armor;
};

struct ArmorStep {
	Fixed delta;

	typedef Armor Data;
	typedef ArmorMemento Memento;

	void apply_step(Data &d, Memento &memento) const;

	void revert_step(Data &d, Memento const &memento) const;
};

Fixed get_damage_after_armor(flecs::entity const &e, octopus::Fixed damage);

}
