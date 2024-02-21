#pragma once

#include <flecs.h>
#include "octopus/components/generic/Components.hh"
#include "octopus/components/basic/Position.hh"
#include "octopus/components/step/Step.hh"

namespace octopus
{

struct Move {
    Position destination;
	bool aggro = false;
	bool enabled = false;
};

struct MoveStep {
	Move data;
};

struct MoveMemento {
	Move data;

	typedef Move Data;
	typedef MoveStep Step;
};

template<>
void apply_step(MoveMemento &m, MoveMemento::Data &d, MoveMemento::Step const &s);

template<>
void revert_memento(MoveMemento::Data &d, MoveMemento const &memento);

} // octopus
