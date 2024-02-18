#include "HitPoint.hh"

namespace octopus
{

template<>
void apply(HitPoint &p, HitPoint::Memento const &v)
{
    p.hp -= v.dmg;
}

template<>
void revert(HitPoint &p, HitPoint::Memento const &v)
{
    p.hp += v.dmg;
}

template<>
void set_no_op(HitPoint::Memento &v)
{
    v.dmg = 0;
}

template<>
void apply_step(HitPointMemento &m, HitPointMemento::Data &d, HitPointMemento::Step const &s)
{
	m.hp = d.hp;
	d.hp += s.delta;
}

template<>
void revert_memento(HitPointMemento::Data &d, HitPointMemento const &m)
{
	d.hp = m.hp;
}

}
