
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

	void apply_step(Data &d, Memento &memento) const;

	void revert_step(Data &d, Memento const &memento) const;
};

}
