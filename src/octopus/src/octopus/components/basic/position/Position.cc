#include "Position.hh"

namespace octopus
{

template<>
void apply_step(PositionStep::Memento &memento, PositionStep::Data &d, PositionStep const &s)
{
	memento.pos = d.pos;
	d.pos += s.delta;
}

template<>
void revert_step<PositionStep>(PositionStep::Data &d, PositionStep::Memento const &memento)
{
	d.pos = memento.pos;
}

}
