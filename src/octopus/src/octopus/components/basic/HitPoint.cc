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
}
