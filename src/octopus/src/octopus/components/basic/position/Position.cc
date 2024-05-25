#include "Position.hh"

namespace octopus
{

///////////////////////////
/// Position STEP
///////////////////////////

void PositionStep::apply_step(Data &d, Memento &memento) const
{
	memento.pos = d.pos;
	d.pos += delta;
}

void PositionStep::revert_step(Data &d, Memento const &memento) const
{
	d.pos = memento.pos;
}

///////////////////////////
/// Mass STEP
///////////////////////////

void MassStep::apply_step(Data &d, Memento &memento) const
{
	memento.old_mass = d.mass;
	d.mass = new_mass;
}

void MassStep::revert_step(Data &d, Memento const &memento) const
{
	d.mass = memento.old_mass;
}

///////////////////////////
/// Velocity STEP
///////////////////////////

void VelocityStep::apply_step(Data &d, Memento &memento) const
{
	memento.old_velocity = d.velocity;
	d.velocity = new_velocity;
}

void VelocityStep::revert_step(Data &d, Memento const &memento) const
{
	d.velocity = memento.old_velocity;
}

}
