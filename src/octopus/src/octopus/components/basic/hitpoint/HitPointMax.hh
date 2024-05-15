
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
};

template<>
void apply_step(HitPointMaxStep::Memento &memento, HitPointMaxStep::Data &d, HitPointMaxStep const &s);

template<>
void revert_step<HitPointMaxStep>(HitPointMaxStep::Data &d, HitPointMaxStep::Memento const &memento);

}
