#include "PositionInTree.hh"

namespace octopus
{

///////////////////////////
/// PositionInTree STEP
///////////////////////////

void PositionInTreeStep::apply_step(Data &d, Memento &memento) const
{
	memento.old_setup = d.idx_leaf[idx_tree];
	d.idx_leaf[idx_tree] = new_setup;
}

void PositionInTreeStep::revert_step(Data &d, Memento const &memento) const
{
	d.idx_leaf[idx_tree] = memento.old_setup;
}

}
