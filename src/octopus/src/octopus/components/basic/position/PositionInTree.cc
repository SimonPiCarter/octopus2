#include "PositionInTree.hh"

namespace octopus
{

///////////////////////////
/// PositionInTree STEP
///////////////////////////

void PositionInTreeStep::apply_step(Data &d, Memento &memento) const
{
	memento.old_setup = d;
	d = new_setup;
}

void PositionInTreeStep::revert_step(Data &d, Memento const &memento) const
{
	d = memento.old_setup;
}

}
