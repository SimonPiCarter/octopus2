#include "Destroyable.hh"

namespace octopus
{

void DestroyableStep::apply_step(Data &d, Memento &memento) const
{
	memento.old_timestamp = d.timestamp;
	d.timestamp = new_timestamp;
}

void DestroyableStep::revert_step(Data &d, Memento const &memento) const
{
	d.timestamp = memento.old_timestamp;
}

}
