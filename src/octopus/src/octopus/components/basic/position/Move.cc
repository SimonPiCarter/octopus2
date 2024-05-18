#include "Move.hh"

namespace octopus
{

void MoveStep::apply_step(Data &d, Memento &memento) const
{
	memento.pos = d.pos;
	d.pos += delta;
}

void MoveStep::revert_step(Data &d, Memento const &memento) const
{
	d.pos = memento.pos;
}

}
