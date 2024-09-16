
#pragma once

#include "octopus/components/step/Step.hh"
#include "octopus/utils/FixedPoint.hh"

namespace octopus
{

struct FlockRef {
	flecs::entity ref;
};

struct Flock {
	uint32_t arrived = 0;
};

struct FlockArrivedMemento {
	uint32_t old_arrived;
};

struct FlockArrivedStep {
	uint32_t new_arrived;

	typedef Flock Data;
	typedef FlockArrivedMemento Memento;

	void apply_step(Data &d, Memento &memento) const;

	void revert_step(Data &d, Memento const &memento) const;
};

}
