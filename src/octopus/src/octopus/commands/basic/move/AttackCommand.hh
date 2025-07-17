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
	bool forced_target = false;
	FlockHandle flock_handle;
	Vector source_pos;
	bool patrol = false;

	static constexpr char const * const naming()  { return "attack"; }
	struct State {};

	static AttackCommand make_patrol_command(Vector const &target) {
		AttackCommand cmd { flecs::entity(), target, true };
		cmd.patrol = true;
		return cmd;
	}
};

struct AttackCommandMemento {
	flecs::entity old_target;
	bool old_forced_target;
};

struct AttackCommandStep {
	flecs::entity new_target;

	typedef AttackCommand Data;
	typedef AttackCommandMemento Memento;

	void apply_step(Data &d, Memento &memento) const
	{
		memento.old_target = d.target;
		memento.old_forced_target = d.forced_target;
		d.target = new_target;
		d.forced_target = false;
	}

	void revert_step(Data &d, Memento const &memento) const
	{
		d.target = memento.old_target;
		d.forced_target = memento.old_forced_target;
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

struct AttackCommandSourcePosMemento {
	Vector old_source;
};

struct AttackCommandSourcePosStep {
	Vector new_source;

	typedef AttackCommand Data;
	typedef AttackCommandSourcePosMemento Memento;

	void apply_step(Data &d, Memento &memento) const
	{
		memento.old_source = d.source_pos;
		d.source_pos = new_source;
	}

	void revert_step(Data &d, Memento const &memento) const
	{
		d.source_pos = memento.old_source;
	}
};

struct AttackCommandTargetPosMemento {
	Vector old_target;
};

struct AttackCommandTargetPosStep {
	Vector new_target;

	typedef AttackCommand Data;
	typedef AttackCommandTargetPosMemento Memento;

	void apply_step(Data &d, Memento &memento) const
	{
		memento.old_target = d.target_pos;
		d.target_pos = new_target;
	}

	void revert_step(Data &d, Memento const &memento) const
	{
		d.target_pos = memento.old_target;
	}
};

/// END State

struct AttackTrigger
{
	flecs::entity target;
};

} // octopus
