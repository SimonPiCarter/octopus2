#pragma once

#include "flecs.h"

#include "octopus/serialization/commands/CommandSupport.hh"
#include "octopus/serialization/queue/CommandQueueSupport.hh"
#include "octopus/systems/input/Input.hh"
#include "octopus/components/basic/flock/FlockManager.hh"
#include "octopus/components/advanced/debuff/DebuffAll.hh"
#include "octopus/world/ability/AbilityTemplateLibrary.hh"
#include "octopus/world/production/ProductionTemplateLibrary.hh"
#include "octopus/world/step/StepEntityManager.hh"

namespace octopus
{

template<typename StepManager_t, typename... tArgs>
void advanced_components_support(flecs::world& ecs)
{
    using command_variant_t = std::variant<tArgs...>;

	basic_commands_support(ecs);
	command_queue_support<tArgs...>(ecs);
  ecs.component<DebuffAll>();

    ecs.component<StepEntityManager>();
    ecs.component<Input<command_variant_t, StepManager_t>>();
    ecs.component<FlockManager>()
		.member("flocks", &FlockManager::flocks)
		.member("last_init", &FlockManager::last_init);
    ecs.component<AbilityTemplateLibrary<StepManager_t>>();
    ecs.component<ProductionTemplateLibrary<StepManager_t>>();
}

} // namespace octopus
