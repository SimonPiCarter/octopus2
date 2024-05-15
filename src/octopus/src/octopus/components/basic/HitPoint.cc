#include "HitPoint.hh"

namespace octopus
{

template<>
void apply_step(HitPointStep::Memento &memento, HitPointStep::Data &d, HitPointStep const &s)
{
	memento.hp = d.qty;
	d.qty += s.delta;
}

template<>
void revert_step<HitPointStep>(HitPointStep::Data &d, HitPointStep::Memento const &memento)
{
	d.qty = memento.hp;
}

}
