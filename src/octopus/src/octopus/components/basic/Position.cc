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

template<bool final, bool alt>
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
		else if(!alt)
		{
			std::swap(s.vec.x, s.vec.y);
			s.vec.x = -s.vec.x;
			position_subsystem<false, true>(grid_p, e, p, s);
		}
		else
		{
			std::swap(s.vec.x, s.vec.y);
			s.vec.y = -s.vec.y;
			position_subsystem<true, false>(grid_p, e, p, s);
		}
    }
    else if(old_i != i || old_j != j)
    {
        set(grid_p, old_i, old_j, flecs::entity());
        set(grid_p, i, j, e);
    }
}

void steering_system(Grid &grid_p, flecs::entity const &e, PositionMemento::Data const &p, PositionMemento::Step &s)
{
	size_t range = 3;
	long long i = long(p.vec.x.to_int());
	long long j = long(p.vec.y.to_int());

	Vector new_pos = p.vec + s.vec;

	flecs::entity_view ent_threat;
	PositionMemento::Data pos_threat;
	// PositionMemento::Step speed_threat;
	Fixed threat_distance = -1;
	for(long long x = std::max<long long>(0, i - range) ; x < i + range && x < grid_p.x ; ++ x)
	{
		for(long long y = std::max<long long>(0, j - range) ; y < j + range && y < grid_p.y ; ++ y)
		{
			flecs::entity_view ent = get(grid_p, x, y);
			if(ent && ent != e)
			{
				ent.get([&](PositionMemento::Data const &po){
					Vector distance_l = new_pos - po.vec;
					Fixed sq_dist_l = square_length(distance_l);
					if(sq_dist_l > 0.01 && sq_dist_l < 2
					&& (!ent_threat || sq_dist_l < threat_distance))
					{
						ent_threat = ent;
						pos_threat = po;
						// speed_threat = so;
						threat_distance = sq_dist_l;
					}
				});
			}
		}
	}

	if(ent_threat)
	{
		Vector avoidance_l = new_pos - pos_threat.vec;
		Fixed length_l = length(avoidance_l);
		if(length_l < 0.01)
		{
			length_l = 0.01;
		}
		avoidance_l /= length_l;
		Fixed orig_length_l = length(s.vec);
		if(orig_length_l < 0.001)
		{
			orig_length_l = 1;
		}
		avoidance_l *= orig_length_l;
		s.vec += avoidance_l;
		Fixed new_length_l = length(s.vec);
		if(new_length_l < 0.001)
		{
			new_length_l = orig_length_l;
		}
		s.vec /= new_length_l / orig_length_l;
	}

    // size_t new_i = size_t((p.vec.x+s.vec.x).to_int());
    // size_t new_j = size_t((p.vec.y+s.vec.y).to_int());
	// if(new_i != i || new_j != j)
    // {
    //     set(grid_p, i, j, flecs::entity());
    //     set(grid_p, new_i, new_j, e);
    // }

	// size_t old_i = size_t(p.vec.x.to_int());
    // size_t old_j = size_t(p.vec.y.to_int());
    // size_t new_i = size_t((p.vec.x+s.vec.x).to_int());
    // size_t new_j = size_t((p.vec.y+s.vec.y).to_int());

    // if(old_i != new_i || old_j != new_j)
    // {
	// 	if(!is_free(grid_p, new_i, new_j))
	// 	{
	// 		s.vec.x = 0;
	// 		s.vec.y = 0;
	// 	}
	// 	else
	// 	{
	// 		set(grid_p, old_i, old_j, flecs::entity());
	// 		set(grid_p, new_i, new_j, e);
	// 	}
	// }
}

void position_system(Grid &grid_p, flecs::entity const &e, PositionMemento::Data const &p, PositionMemento::Step &s)
{
	steering_system(grid_p, e, p, s);
	position_subsystem<false, false>(grid_p, e, p, s);
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
