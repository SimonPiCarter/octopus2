#include "ProductionQueue.hh"

namespace octopus
{

void ProductionQueueTimestampStep::apply_step(Data &d, Memento &m) const
{
	m.old_timestamp = d.start_timestamp;
	d.start_timestamp = new_timestamp;
}

void ProductionQueueTimestampStep::revert_step(Data &d, Memento const &m) const
{
	d.start_timestamp = m.old_timestamp;
}

void ProductionQueueAddStep::apply_step(Data &d, Memento &) const
{
	d.queue.push_back(production);
}

void ProductionQueueAddStep::revert_step(Data &d, Memento const &) const
{
	d.queue.pop_back();
}

void ProductionQueueCancelStep::apply_step(Data &d, Memento &m) const
{
	m.old = d;
	d.queue.erase(d.queue.begin()+idx);
	if(idx == 0)
	{
		d.start_timestamp = 0;
	}
}

void ProductionQueueCancelStep::revert_step(Data &d, Memento const &m) const
{
	d = m.old;
}

} // namespace octopus
