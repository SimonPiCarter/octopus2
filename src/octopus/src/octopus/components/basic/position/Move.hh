
#pragma once

#include "octopus/components/step/Step.hh"
#include "octopus/utils/Vector.hh"

namespace octopus
{

struct Move {
	Vector pos;
	Fixed speed = Fixed(1);
};

struct MoveMemento {
	Vector pos;
};

struct MoveStep {
	Vector delta;

	typedef Move Data;
	typedef MoveMemento Memento;

	void apply_step(Data &d, Memento &memento) const;

	void revert_step(Data &d, Memento const &memento) const;
};

}
