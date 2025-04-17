#pragma once

#include "octopus/components/basic/rally_point/RallyPoint.hh"

namespace octopus
{

/// @brief A command used to set a rally point
struct SetRallyPointCommand {
	RallyPoint rally_point;

	static constexpr char const * const naming()  { return "set_rally_point"; }
	struct State {};
};

template<class StepManager_t, class CommandQueue_t>
void set_up_rally_point_command_system(flecs::world &ecs, StepManager_t &manager)
{
	ecs.system<SetRallyPointCommand const, CommandQueue_t>()
		.kind(ecs.entity(PostUpdateUnpausedPhase))
		.with(CommandQueue_t::state(ecs), ecs.component<SetRallyPointCommand::State>())
		.each([&ecs, &manager](flecs::entity e, SetRallyPointCommand const &rally_point_command, CommandQueue_t &queue) {
			manager.get_last_layer().back().template get<RallyPointStep>().add_step(e, {rally_point_command.rally_point});
			queue._queuedActions.push_back(CommandQueueActionDone());
		});
}

}
