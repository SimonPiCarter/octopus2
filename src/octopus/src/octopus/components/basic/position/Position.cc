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

///////////////////////////
/// Collision STEP
///////////////////////////

void CollisionStep::apply_step(Data &d, Memento &memento) const
{
	memento.old_collision = d.collision;
	d.collision = new_collision;
}

void CollisionStep::revert_step(Data &d, Memento const &memento) const
{
	d.collision = memento.old_collision;
}


///////////////////////////
/// StuckInfo STEP
///////////////////////////


void StuckInfoStep::apply_step(Data &d, Memento &memento) const
{
	memento.old_info = d.stuck_info;
	d.stuck_info = new_info;
}

void StuckInfoStep::revert_step(Data &d, Memento const &memento) const
{
	d.stuck_info = memento.old_info;
}

}
