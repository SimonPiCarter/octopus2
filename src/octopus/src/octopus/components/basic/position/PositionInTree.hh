
#pragma once

#include "octopus/components/step/Step.hh"
#include "octopus/utils/Vector.hh"

namespace octopus
{

struct PositionInTree {
	int32_t idx_leaf = -1;
};

///////////////////////////
/// PositionInTree STEP
///////////////////////////

struct PositionInTreeMemento {
	PositionInTree old_setup;
};

struct PositionInTreeStep {
	PositionInTree new_setup;

	typedef PositionInTree Data;
	typedef PositionInTreeMemento Memento;

	void apply_step(Data &d, Memento &memento) const;

	void revert_step(Data &d, Memento const &memento) const;
};

}
