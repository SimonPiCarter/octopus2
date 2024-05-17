#include "Position.hh"

namespace octopus
{

void PositionStep::apply_step(Data &d, Memento &memento) const
{
	memento.pos = d.pos;
	d.pos += delta;
}

void PositionStep::revert_step(Data &d, Memento const &memento) const
{
	d.pos = memento.pos;
}

}
