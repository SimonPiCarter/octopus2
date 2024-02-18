#include "Target.hh"

#include "octopus/utils/Grid.hh"
#include "octopus/components/basic/Position.hh"
#include "octopus/components/basic/Team.hh"

#include <iostream>

namespace octopus
{

template<>
void apply(Target &p, Target::Memento const &v)
{
    if(!v.no_op) p.data = v.cur;
}

template<>
void revert(Target &p, Target::Memento const &v)
{
    if(!v.no_op) p.data = v.old;
}

template<>
void set_no_op(Target::Memento &v)
{
    v.no_op = true;
}

void target_system(Grid const &grid_p, flecs::entity e, Position const & p, Target const& z, Target::Memento& zm, Team const &t)
{
	zm.old = z.data;
	zm.cur = z.data;

	long long i = long(p.vec.x.to_int());
	long long j = long(p.vec.y.to_int());

	flecs::entity ent_target;
	Vector target;
	Fixed best_diff = -1;
	for(long long x = std::max<long long>(0, i - z.data.range) ; x < i+z.data.range && x < grid_p.x ; ++ x)
	{
		for(long long y = std::max<long long>(0, j - z.data.range) ; y < j+z.data.range && y < grid_p.y ; ++ y)
		{
			if(x == i && y == j) { continue; }
			flecs::entity ent = get(grid_p, x, y);
			if(ent && ent != e)
			{
				Position const *pos = ent.get<Position>();
				Team const *team = ent.get<Team>();
				if(pos && team
				&& team->id != t.id)
				{
					Fixed diff = square_length(p.vec - pos->vec);
					if(best_diff < 0 || diff < best_diff)
					{
						std::swap(diff, best_diff);
						target = pos->vec;
						ent_target = ent;
					}
				}
			}
		}
	}

	if(ent_target)
	{
		zm.cur.target = ent_target;
	}

	zm.no_op = zm.cur.target == zm.old.target;
}

template<>
void apply_step(TargetMemento &m, TargetMemento::Data &d, TargetMemento::Step const &s)
{
	std::swap(m.data, d.data);
	d.data = s.data;
}

template<>
void revert_memento(TargetMemento::Data &d, TargetMemento const &memento)
{
	d.data = memento.data;
}

} // octopus
