#pragma once

#include "flecs.h"
#include "octopus/utils/Vector.hh"
#include "octopus/components/basic/attack/Attack.hh"
#include "octopus/components/basic/flock/Flock.hh"
#include "octopus/components/basic/position/Position.hh"
#include "octopus/components/basic/position/Move.hh"
#include "octopus/components/basic/player/Team.hh"
#include "octopus/components/basic/timestamp/TimeStamp.hh"
#include "octopus/commands/queue/CommandQueue.hh"
#include "octopus/commands/basic/move/MoveCommand.hh"
#include "octopus/world/position/closest_neighbours.hh"
#include "octopus/world/position/PositionContext.hh"
#include "octopus/world/stats/TimeStats.hh"

#include "octopus/world/path/direction.hh"

namespace octopus
{

///////////////////////////////
/// State					 //
///////////////////////////////

struct AttackCommand {
	flecs::entity target;
	Vector target_pos;
	bool move = false;
	bool init = false;
	FlockHandle flock_handle;

	static constexpr char const * const naming()  { return "attack"; }
	struct State {};
};

struct AttackCommandMemento {
	flecs::entity old_target;
};

struct AttackCommandStep {
	flecs::entity new_target;

	typedef AttackCommand Data;
	typedef AttackCommandMemento Memento;

	void apply_step(Data &d, Memento &memento) const
	{
		memento.old_target = d.target;
		d.target = new_target;
	}

	void revert_step(Data &d, Memento const &memento) const
	{
		d.target = memento.old_target;
	}
};

struct AttackCommandInitMemento {
	bool old_init = false;
};

struct AttackCommandInitStep {
	bool new_init = false;

	typedef AttackCommand Data;
	typedef AttackCommandInitMemento Memento;

	void apply_step(Data &d, Memento &memento) const
	{
		memento.old_init = d.init;
		d.init = new_init;
	}

	void revert_step(Data &d, Memento const &memento) const
	{
		d.init = memento.old_init;
	}
};

/// END State

struct AttackTrigger
{
	flecs::entity target;
};

} // octopus
