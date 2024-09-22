#pragma once

#include <string>
#include <vector>

namespace octopus
{

struct ProductionQueue
{
    int64_t start_timestamp = 0;
    std::vector<std::string> queue;
};

struct ProductionQueueTimestampMemento {
    int64_t old_timestamp = 0;
};

struct ProductionQueueTimestampStep {
    int64_t new_timestamp = 0;

	typedef ProductionQueue Data;
	typedef ProductionQueueTimestampMemento Memento;

	void apply_step(Data &d, Memento &memento) const;

	void revert_step(Data &d, Memento const &memento) const;
};

struct ProductionQueueAddMemento {
};

struct ProductionQueueAddStep {
	std::string production;

	typedef ProductionQueue Data;
	typedef ProductionQueueAddMemento Memento;

	void apply_step(Data &d, Memento &memento) const;

	void revert_step(Data &d, Memento const &memento) const;
};

struct ProductionQueueCancelMemento {
    ProductionQueue old;
};

struct ProductionQueueCancelStep {
	int idx = 0;

	typedef ProductionQueue Data;
	typedef ProductionQueueCancelMemento Memento;

	void apply_step(Data &d, Memento &memento) const;

	void revert_step(Data &d, Memento const &memento) const;
};


} // namespace octopus
