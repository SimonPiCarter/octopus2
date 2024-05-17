#include "HitPointMax.hh"

namespace octopus
{

void HitPointMaxStep::apply_step(Data &d, Memento &memento) const
{
	memento.hp = d.qty;
	d.qty += delta;
}

void HitPointMaxStep::revert_step(Data &d, Memento const &memento) const
{
	d.qty = memento.hp;
}

}
