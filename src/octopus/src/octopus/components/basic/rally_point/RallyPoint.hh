#pragma once

#include "flecs.h"
#include "octopus/utils/FixedPoint.hh"
#include "octopus/utils/Vector.hh"

namespace octopus
{
struct RallyPoint
{
	Vector target;
	Fixed tolerance;
	bool enabled = false;
};

struct RallyPointMemento {
    RallyPoint old_rally_point;
};

struct RallyPointStep {
    RallyPoint new_rally_point;

	typedef RallyPoint Data;
	typedef RallyPointMemento Memento;

	void apply_step(Data &d, Memento &memento) const;

	void revert_step(Data &d, Memento const &memento) const;
};

/// @brief declare component
void declare_rally_points(flecs::world &ecs);

} // namespace octopus


