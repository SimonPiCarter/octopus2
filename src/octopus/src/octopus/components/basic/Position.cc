#include "Position.hh"

#include "octopus/utils/Grid.hh"

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

void position_system(Grid &grid_p, flecs::entity e, Position const & p, Velocity &v)
{
    size_t old_i = size_t(p.vec.x.to_int());
    size_t old_j = size_t(p.vec.y.to_int());
    size_t i = size_t((p.vec.x+v.vec.x).to_int());
    size_t j = size_t((p.vec.y+v.vec.y).to_int());

    if((old_i != i || old_j != j)
    && !is_free(grid_p, i, j))
    {
        v.vec.x = 0;
        v.vec.y = 0;
    }
    else if(old_i != i || old_j != j)
    {
        set(grid_p, old_i, old_j, flecs::entity());
        set(grid_p, i, j, e);
    }
}

} // octopus
