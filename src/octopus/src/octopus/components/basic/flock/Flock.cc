#include "Flock.hh"

namespace octopus
{

void FlockArrivedStep::apply_step(Data &d, Memento &memento) const
{
	memento.old_arrived = d.arrived;
	d.arrived = new_arrived;
}

void FlockArrivedStep::revert_step(Data &d, Memento const &memento) const
{
	d.arrived = memento.old_arrived;
}

}
