#include "Move.hh"

#include "octopus/utils/Grid.hh"
#include "octopus/components/basic/Position.hh"
#include "octopus/components/basic/Team.hh"
#include "octopus/components/step/StepContainer.hh"
#include "octopus/components/behaviour/target/Target.hh"

#include <iostream>

namespace octopus
{

template<>
void apply_step(MoveMemento &m, MoveMemento::Data &d, MoveMemento::Step const &s)
{
	std::swap(m.data, d);
	d = s.data;
}

template<>
void revert_memento(MoveMemento::Data &d, MoveMemento const &memento)
{
	d = memento.data;
}

} // octopus
