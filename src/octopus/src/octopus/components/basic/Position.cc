#include "Position.hh"

namespace octopus
{
template<>
void apply(Position &p, Position::Memento const &v)
{
    p.vec += v.vec;
}

template<>
void revert(Position &p, Position::Memento const &v)
{
    p.vec -= v.vec;
}

template<>
void set_no_op(Position::Memento &v)
{
    v.vec.x = 0;
    v.vec.y = 0;
}
}