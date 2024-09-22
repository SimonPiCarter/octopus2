#pragma once

#include <functional>
#include "flecs.h"

namespace octopus
{
struct EntityCreationStep
{
	std::function<void(flecs::entity, flecs::world const &)> set_up_function;
};

struct EntityCreationMemento
{
	flecs::entity created_entity;
};

void apply_entity_creation_step(flecs::world &ecs, EntityCreationStep const &step, EntityCreationMemento &memento_p);
void revert_entity_creation_step(flecs::world &ecs, EntityCreationMemento const &memento_p);

} // namespace octopus
