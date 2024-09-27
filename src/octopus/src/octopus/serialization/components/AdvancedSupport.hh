#pragma once

#include "flecs.h"

#include "octopus/serialization/commands/CommandSupport.hh"
#include "octopus/serialization/queue/CommandQueueSupport.hh"
#include "octopus/systems/input/Input.hh"

namespace octopus
{

template<typename StepManager_t, typename... tArgs>
void advanced_components_support(flecs::world& ecs)
{
    using command_variant_t = std::variant<tArgs...>;

	basic_commands_support(ecs);
	command_queue_support<tArgs...>(ecs);

    ecs.component<Input<command_variant_t, StepManager_t>>();
}

} // namespace octopus
