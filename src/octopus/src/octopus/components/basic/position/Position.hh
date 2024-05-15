
#pragma once

#include "octopus/components/step/Step.hh"
#include "octopus/utils/Vector.hh"

// HP

namespace octopus
{

struct Position {
	Vector pos;
};

struct PositionMemento {
	Vector pos;
};

struct PositionStep {
	Vector delta;

	typedef Position Data;
	typedef PositionMemento Memento;
};

template<>
void apply_step(PositionStep::Memento &memento, PositionStep::Data &d, PositionStep const &s);

template<>
void revert_step<PositionStep>(PositionStep::Data &d, PositionStep::Memento const &memento);

}
