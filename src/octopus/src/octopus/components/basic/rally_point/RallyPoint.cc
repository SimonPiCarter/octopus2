#include "RallyPoint.hh"

namespace octopus
{

void RallyPointStep::apply_step(Data &d, Memento &memento) const
{
	memento.old_rally_point = d;
	d = new_rally_point;
}

void RallyPointStep::revert_step(Data &d, Memento const &memento) const
{
	d = memento.old_rally_point;
}

void declare_rally_points(flecs::world &ecs)
{
	ecs.component<RallyPoint>()
		.member("target", &RallyPoint::target)
		.member("tolerance", &RallyPoint::tolerance)
		.member("enabled", &RallyPoint::enabled)
	;
}

} // namespace octopus

