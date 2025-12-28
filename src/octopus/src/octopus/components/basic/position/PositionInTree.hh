
#pragma once

#include <array>

#include "octopus/components/step/Step.hh"
#include "octopus/utils/Vector.hh"

namespace octopus
{

struct PositionInTree {
	std::array<int32_t, 3> idx_leaf = {-1,-1,-1};
};

struct ForcePositionInTree { bool decoy = false; };

///////////////////////////
/// PositionInTree STEP
///////////////////////////

struct PositionInTreeMemento {
	int32_t old_setup = 0;
};

struct PositionInTreeStep {
	int32_t new_setup = 0;
	uint32_t const idx_tree = 0;

	typedef PositionInTree Data;
	typedef PositionInTreeMemento Memento;

	void apply_step(Data &d, Memento &memento) const;

	void revert_step(Data &d, Memento const &memento) const;
};

}
