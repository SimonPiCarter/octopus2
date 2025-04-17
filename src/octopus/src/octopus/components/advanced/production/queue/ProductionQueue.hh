#pragma once

#include <string>
#include <vector>
#include "octopus/utils/Vector.hh"

namespace octopus
{

struct ProductionQueue
{
	int64_t start_timestamp = 0;
	std::vector<std::string> queue;
	/// @brief point where unit should spawn
	Vector spawn_point = Vector(0,4);
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

struct ProductionQueueOperationMemento {
	std::vector<std::string> old_queue;
};

struct ProductionQueueOperationStep {
	// empty to not add anything
	std::string added_production;
	// < 0 to no cancel anything
	int canceled_idx = -1;

	typedef ProductionQueue Data;
	typedef ProductionQueueOperationMemento Memento;

	void apply_step(Data &d, Memento &memento) const;

	void revert_step(Data &d, Memento const &memento) const;
};

} // namespace octopus
