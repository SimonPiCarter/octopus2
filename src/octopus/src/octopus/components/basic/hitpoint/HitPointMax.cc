#include "HitPointMax.hh"

namespace octopus
{

template<>
void apply_step(HitPointMaxStep::Memento &memento, HitPointMaxStep::Data &d, HitPointMaxStep const &s)
{
	memento.hp = d.qty;
	d.qty += s.delta;
}

template<>
void revert_step<HitPointMaxStep>(HitPointMaxStep::Data &d, HitPointMaxStep::Memento const &memento)
{
	d.qty = memento.hp;
}

}
