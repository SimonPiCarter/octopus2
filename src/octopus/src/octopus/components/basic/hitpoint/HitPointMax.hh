
#pragma once

#include "octopus/components/step/Step.hh"
#include "octopus/utils/FixedPoint.hh"

// HP max

namespace octopus
{

struct HitPointMax {
	Fixed qty;
};

struct HitPointMaxMemento {
	Fixed hp;
};

struct HitPointMaxStep {
	Fixed delta;

	typedef HitPointMax Data;
	typedef HitPointMaxMemento Memento;

	void apply_step(Data &d, Memento &memento) const;

	void revert_step(Data &d, Memento const &memento) const;
};

}
