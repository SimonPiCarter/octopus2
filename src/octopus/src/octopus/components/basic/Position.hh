#pragma once

#include "octopus/components/generic/Components.hh"
#include "octopus/utils/FixedPoint.hh"
// Position

namespace octopus
{

struct Velocity {
    Fixed x, y;
};

struct Position {
    Fixed x, y;

    typedef Velocity Memento;
};

template<>
void apply(Position &p, Position::Memento const &v);

template<>
void revert(Position &p, Position::Memento const &v);

template<>
void set_no_op(Position::Memento &v);

} // octopus
