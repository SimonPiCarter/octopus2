#include "Target.hh"

#include "octopus/utils/Grid.hh"
#include "octopus/components/basic/Position.hh"
#include "octopus/components/basic/Team.hh"

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

void target_system(Grid const &grid_p, Position const & p, Velocity &v, Target const& z, TargetMemento& zm, Team const &t)
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
			flecs::entity e = get(grid_p, x, y);
			if(e)
			{
				Position const *pos = e.get<Position>();
				Team const *team = e.get<Team>();
				if(pos && team
				&& team->id != t.id)
				{
					Fixed diff = square_length(p.vec - pos->vec);
					if(best_diff < 0 || diff < best_diff)
					{
						std::swap(diff, best_diff);
						target = pos->vec;
						ent_target = e;
					}
				}
			}
		}
	}

	if(best_diff >= 0)
	{
		zm.cur.target = ent_target;
	}

	if(zm.cur.target)
	{
		Position const *target_pos = zm.cur.target.get<Position>();
		if(target_pos)
		{
			v.vec = target_pos->vec - p.vec;
			v.vec /= length(v.vec);
		}
	}

	zm.no_op = zm.cur.target == zm.old.target;
}

} // octopus
