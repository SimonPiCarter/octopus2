#include "Position.hh"

namespace octopus
{
template<>
void apply(Position &p, Position::Memento const &v)
{
    p.x += v.x;
    p.y += v.y;
}

template<>
void revert(Position &p, Position::Memento const &v)
{
    p.x -= v.x;
    p.y -= v.y;
}

template<>
void set_no_op(Position::Memento &v)
{
    v.x = 0;
    v.y = 0;
}
}