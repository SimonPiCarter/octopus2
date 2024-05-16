#pragma once

namespace octopus
{

// struct CommandQueueActionDone

// template<typename variant_t>
// struct CommandQueueActionReplace

// template<typename variant_t>
// struct CommandQueueActionAddFront

// template<typename variant_t>
// struct CommandQueueActionAddBack

template<typename variant_t>
void handle_action(CommandQueue<variant_t> &queue_p, CommandQueueActionDone const &action_p)
{
	queue_p._done = true;
}

template<typename variant_t>
void handle_action(CommandQueue<variant_t> &queue_p, CommandQueueActionReplace<variant_t> const &action_p)
{
	queue_p._queued = action_p._queued;
}

template<typename variant_t>
void handle_action(CommandQueue<variant_t> &queue_p, CommandQueueActionAddFront<variant_t> const &action_p)
{
	for(auto && action : action_p._queued)
		queue_p._queued.push_front(action);
}

template<typename variant_t>
void handle_action(CommandQueue<variant_t> &queue_p, CommandQueueActionAddBack<variant_t> const &action_p)
{
	for(auto &&action : action_p._queued)
		queue_p._queued.push_back(action);
}

template<typename variant_t>
void set_up_command_queue_systems(flecs::world &ecs)
{
	// set up relations
    CommandQueue<variant_t>::state(ecs).add(flecs::Exclusive);
    CommandQueue<variant_t>::cleanup(ecs).add(flecs::Exclusive);

	// Apply actions
	ecs.system<CommandQueue<variant_t>>()
		.kind(flecs::PostLoad)
		.each([&ecs](flecs::entity e, CommandQueue<variant_t> &queue_p) {
			if(queue_p._queuedActions.size() > 0)
			{
				/// @todo store queue state
			}
			for(typename CommandQueue<variant_t>::CommandQueueAction const & action_l : queue_p._queuedActions)
			{
				std::visit([&queue_p](auto&& arg)
				{
					handle_action(queue_p, arg);
				}, action_l);
			}

			// clear once done
			queue_p._queuedActions.clear();
		});

	ecs.system<CommandQueue<variant_t>>()
		.kind(flecs::PostLoad)
		.write(CommandQueue<variant_t>::state(ecs), flecs::Wildcard)
		.write(CommandQueue<variant_t>::cleanup(ecs), flecs::Wildcard)
		.each([&ecs](flecs::entity e, CommandQueue<variant_t> &queue_p) {
			queue_p.clean_up_current(ecs, e);
		});

	ecs.system<CommandQueue<variant_t>>()
		.kind(flecs::OnUpdate)
		.write(CommandQueue<variant_t>::state(ecs), flecs::Wildcard)
		.write(CommandQueue<variant_t>::cleanup(ecs), flecs::Wildcard)
		.each([&ecs](flecs::entity e, CommandQueue<variant_t> &queue_p) {
			queue_p.update_current(ecs, e);
		});
}

}
