
#pragma once

#include "octopus/components/step/Step.hh"
#include "octopus/utils/Vector.hh"

namespace octopus
{

/// @brief Structure to allow tracking for
/// stuck entities
struct StuckInfo {
	uint8_t step_stuck = 0;
	Vector last_pos;
};

struct Position {
	Vector pos;
	Vector velocity;
	Fixed mass = Fixed::One();
	Fixed ray = 0.5;
	bool collision = true;
	StuckInfo stuck_info;
};

///////////////////////////
/// Position STEP
///////////////////////////

struct PositionMemento {
	Vector pos;
};

struct PositionStep {
	Vector delta;

	typedef Position Data;
	typedef PositionMemento Memento;

	void apply_step(Data &d, Memento &memento) const;

	void revert_step(Data &d, Memento const &memento) const;
};

///////////////////////////
/// Mass STEP
///////////////////////////

struct MassMemento {
	Fixed old_mass;
};

struct MassStep {
	Fixed new_mass;

	typedef Position Data;
	typedef MassMemento Memento;

	void apply_step(Data &d, Memento &memento) const;

	void revert_step(Data &d, Memento const &memento) const;
};

///////////////////////////
/// Velocity STEP
///////////////////////////

struct VelocityMemento {
	Vector old_velocity;
};

struct VelocityStep {
	Vector new_velocity;

	typedef Position Data;
	typedef VelocityMemento Memento;

	void apply_step(Data &d, Memento &memento) const;

	void revert_step(Data &d, Memento const &memento) const;
};

///////////////////////////
/// Collision STEP
///////////////////////////

struct CollisionMemento {
	bool old_collision;
};

struct CollisionStep {
	bool new_collision;

	typedef Position Data;
	typedef CollisionMemento Memento;

	void apply_step(Data &d, Memento &memento) const;

	void revert_step(Data &d, Memento const &memento) const;
};

///////////////////////////
/// StuckInfo STEP
///////////////////////////

struct StuckInfoMemento {
	StuckInfo old_info;
};

struct StuckInfoStep {
	StuckInfo new_info;

	typedef Position Data;
	typedef StuckInfoMemento Memento;

	void apply_step(Data &d, Memento &memento) const;

	void revert_step(Data &d, Memento const &memento) const;
};


}
