#pragma once

#include "octopus/components/generic/Components.hh"
#include "octopus/utils/Vector.hh"
// Position

namespace octopus
{

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

} // octopus
