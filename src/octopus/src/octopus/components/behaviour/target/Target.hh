#pragma once

#include <flecs.h>
#include "octopus/components/generic/Components.hh"
#include "octopus/components/step/Step.hh"

///
/// State       To State            Note
/// Lookup      Chasing             found target
/// Chasing     Attacking           in range
/// Chasing     Lookup              target dead or out of sight
/// Attacking   Lookup              target dead or out of sight
/// Attacking   Chasing             target our of range
/// None        All                 other state running
/// All         None                other state running
///

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

struct TargetMemento2 {
    TargetData old;
    TargetData cur;
    bool no_op = true;
};

struct Target {
    TargetData data;
    typedef TargetMemento2 Memento;
};

void target_system(Grid const &grid_p, flecs::entity e, Position const & p, Target const& z, Target::Memento& zm, Team const &t);

struct TargetStep {
	TargetData data;
};

struct TargetMemento {
	TargetData data;

	typedef Target Data;
	typedef TargetStep Step;
};

template<>
void apply_step(TargetMemento &m, TargetMemento::Data &d, TargetMemento::Step const &s);

template<>
void revert_memento(TargetMemento::Data &d, TargetMemento const &memento);

} // octopus
