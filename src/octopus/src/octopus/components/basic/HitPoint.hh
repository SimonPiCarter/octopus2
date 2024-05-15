
#pragma once

#include "octopus/components/generic/Components.hh"
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
};

template<>
void apply_step(HitPointStep::Memento &memento, HitPointStep::Data &d, HitPointStep const &s);

template<>
void revert_step<HitPointStep>(HitPointStep::Data &d, HitPointStep::Memento const &memento);

}
