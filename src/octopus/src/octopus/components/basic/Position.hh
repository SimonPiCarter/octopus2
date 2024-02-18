#pragma once

#include "flecs.h"

#include "octopus/components/generic/Components.hh"
#include "octopus/components/step/Step.hh"
#include "octopus/utils/Vector.hh"

// Position

namespace octopus
{

struct Grid;

struct Velocity {
	Vector vec;
};

struct Position {
	Vector vec;
	Fixed speed = 1;

    typedef Velocity Memento;
};

template<>
void apply(Position &p, Position::Memento const &v);

template<>
void revert(Position &p, Position::Memento const &v);

template<>
void set_no_op(Position::Memento &v);

struct PositionStep {
	Vector vec;
};

struct PositionMemento {
	Vector vec;

	typedef Position Data;
	typedef PositionStep Step;
};

void position_system(Grid &grid_p, flecs::entity const &e, PositionMemento::Data const &p, PositionMemento::Step &s);

template<>
void apply_step(PositionMemento &m, PositionMemento::Data &d, PositionMemento::Step const &s);

template<>
void revert_memento(PositionMemento::Data &d, PositionMemento const &memento);

} // octopus
