#include "Projectile.hh"

namespace octopus
{

void ProjectileStep::apply_step(Data &d, Memento &memento) const
{
	memento.old_pos_target = d.pos_target;
	d.pos_target = new_pos_target;
}

void ProjectileStep::revert_step(Data &d, Memento const &memento) const
{
	d.pos_target = memento.old_pos_target;
}

}
