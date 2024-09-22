#include "EntityCreationStep.hh"

namespace octopus
{

void apply_entity_creation_step(flecs::world &ecs, EntityCreationStep const &step, EntityCreationMemento &memento_p)
{
	flecs::entity e = ecs.entity();
	memento_p.created_entity = e;
	step.set_up_function(e, ecs);
}

void revert_entity_creation_step(flecs::world &ecs, EntityCreationMemento const &memento_p)
{
	memento_p.created_entity.destruct();
}

} // namespace octopus
