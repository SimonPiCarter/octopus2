
#pragma once

#include "octopus/components/step/Step.hh"
#include "octopus/utils/Vector.hh"

namespace octopus
{

struct Position {
	Vector pos;
	Vector velocity;
	Fixed mass = Fixed::One();
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

}
