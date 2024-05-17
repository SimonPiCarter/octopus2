#include "HitPoint.hh"

namespace octopus
{

void HitPointStep::apply_step(Data &d, Memento &memento) const
{
	memento.hp = d.qty;
	d.qty += delta;
}

void HitPointStep::revert_step(Data &d, Memento const &memento) const
{
	d.qty = memento.hp;
}

}
