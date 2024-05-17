
#pragma once

#include "octopus/components/step/Step.hh"
#include "octopus/utils/FixedPoint.hh"

// HP

namespace octopus
{

struct HitPoint {
	Fixed qty;
};

struct HitPointMemento {
	Fixed hp;
};

struct HitPointStep {
	Fixed delta;

	typedef HitPoint Data;
	typedef HitPointMemento Memento;

	void apply_step(Data &d, Memento &memento) const;

	void revert_step(Data &d, Memento const &memento) const;
};

}
