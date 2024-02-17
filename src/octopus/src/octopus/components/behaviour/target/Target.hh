#pragma once

#include <flecs.h>
#include "octopus/components/generic/Components.hh"

namespace octopus
{

class Grid;
struct Position;
struct Team;
struct Velocity;

struct TargetData {
    flecs::entity target;
    uint32_t range = 6;
};

struct TargetMemento {
    TargetData old;
    TargetData cur;
    bool no_op = true;
};

struct Target {
    TargetData data;
    typedef TargetMemento Memento;
};

void target_system(Grid const &grid_p, flecs::entity e, Position const & p, Velocity &v, Target const& z, TargetMemento& zm, Team const &t);

} // octopus
