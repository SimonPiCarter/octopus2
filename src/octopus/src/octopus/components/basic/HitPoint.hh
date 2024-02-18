
#pragma once

#include "octopus/components/generic/Components.hh"
#include "octopus/components/step/Step.hh"
#include "octopus/utils/FixedPoint.hh"

// HP

namespace octopus
{

struct Damage {
    Fixed dmg;
};

struct HitPoint {
    Fixed hp = 10;

    typedef Damage Memento;
};

template<>
void apply(HitPoint &p, HitPoint::Memento const &v);

template<>
void revert(HitPoint &p, HitPoint::Memento const &v);

template<>
void set_no_op(HitPoint::Memento &v);

struct HitPointStep {
	Fixed delta;
};

struct HitPointMemento {
	Fixed hp;

	typedef HitPoint Data;
	typedef HitPointStep Step;
};

template<>
void apply_step(HitPointMemento &m, HitPointMemento::Data &d, HitPointMemento::Step const &s);

template<>
void revert_memento(HitPointMemento::Data &d, HitPointMemento const &memento);

}
