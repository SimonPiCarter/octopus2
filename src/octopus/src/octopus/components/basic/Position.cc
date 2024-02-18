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

template<bool final>
void position_subsystem(Grid &grid_p, flecs::entity const &e, PositionMemento::Data const &p, PositionMemento::Step &s)
{
    size_t old_i = size_t(p.vec.x.to_int());
    size_t old_j = size_t(p.vec.y.to_int());
    size_t i = size_t((p.vec.x+s.vec.x).to_int());
    size_t j = size_t((p.vec.y+s.vec.y).to_int());

    if((old_i != i || old_j != j)
    && !is_free(grid_p, i, j))
    {
		if(final)
		{
			s.vec.x = 0;
			s.vec.y = 0;
		}
		else
		{
			std::swap(s.vec.x, s.vec.y);
			s.vec.x = -s.vec.x;
			position_subsystem<true>(grid_p, e, p, s);
		}
    }
    else if(old_i != i || old_j != j)
    {
        set(grid_p, old_i, old_j, flecs::entity());
        set(grid_p, i, j, e);
    }
}

void position_system(Grid &grid_p, flecs::entity const &e, PositionMemento::Data const &p, PositionMemento::Step &s)
{
	position_subsystem<false>(grid_p, e, p, s);
}

template<>
void apply_step(PositionMemento &m, PositionMemento::Data &d, PositionMemento::Step const &s)
{
	m.vec = d.vec;
	d.vec += s.vec;
}

template<>
void revert_memento(PositionMemento::Data &d, PositionMemento const &m)
{
	d.vec = m.vec;
}

} // octopus
