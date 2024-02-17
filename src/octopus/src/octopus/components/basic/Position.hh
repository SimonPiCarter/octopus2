#pragma once

#include "flecs.h"

#include "octopus/components/generic/Components.hh"
#include "octopus/utils/Vector.hh"

// Position

namespace octopus
{

class Grid;

struct Velocity {
	Vector vec;
};

struct Position {
	Vector vec;

    typedef Velocity Memento;
};

template<>
void apply(Position &p, Position::Memento const &v);

template<>
void revert(Position &p, Position::Memento const &v);

template<>
void set_no_op(Position::Memento &v);

void position_system(Grid &grid_p, flecs::entity e, Position const & p, Velocity &v);

} // octopus
