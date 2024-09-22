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

void ProductionQueueOperationStep::apply_step(Data &d, Memento &m) const
{
	m.old_queue = d.queue;
	if(canceled_idx >= 0 && canceled_idx < d.queue.size())
	{
		d.queue.erase(d.queue.begin()+canceled_idx);
	}
	if(added_production != "")
	{
		d.queue.push_back(added_production);
	}
}

void ProductionQueueOperationStep::revert_step(Data &d, Memento const &m) const
{
	d.queue = m.old_queue;
}

} // namespace octopus
