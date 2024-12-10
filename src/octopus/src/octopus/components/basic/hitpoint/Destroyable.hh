
#pragma once

#include "octopus/components/step/Step.hh"
#include "octopus/utils/FixedPoint.hh"

namespace octopus
{

struct Created {};
struct Destroyed {};

struct Destroyable {
	int64_t timestamp = 0;
};

struct DestroyableMemento {
	int64_t old_timestamp = 0;
};

struct DestroyableStep {
	int64_t new_timestamp;

	typedef Destroyable Data;
	typedef DestroyableMemento Memento;

	void apply_step(Data &d, Memento &memento) const;

	void revert_step(Data &d, Memento const &memento) const;
};

}
