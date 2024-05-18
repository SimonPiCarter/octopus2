#include "Attack.hh"

namespace octopus
{

void AttackWindupStep::apply_step(Data &d, Memento &memento) const
{
	memento.old_windup = d.windup;
	d.windup = new_windup;
}

void AttackWindupStep::revert_step(Data &d, Memento const &memento) const
{
	d.windup = memento.old_windup;
}

void AttackReloadStep::apply_step(Data &d, Memento &memento) const
{
	memento.old_reload = d.reload;
	d.reload = new_reload;
}

void AttackReloadStep::revert_step(Data &d, Memento const &memento) const
{
	d.reload = memento.old_reload;
}

}
