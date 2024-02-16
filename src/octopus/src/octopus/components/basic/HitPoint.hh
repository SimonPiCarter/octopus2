
#pragma once

#include "octopus/components/generic/Components.hh"
#include "octopus/utils/FixedPoint.hh"

// HP

namespace octopus
{

struct Damage {
    Fixed dmg;
};

struct HitPoint {
    Fixed hp;

    typedef Damage Memento;
};

template<>
void apply(HitPoint &p, HitPoint::Memento const &v);

template<>
void revert(HitPoint &p, HitPoint::Memento const &v);

template<>
void set_no_op(HitPoint::Memento &v);

}
