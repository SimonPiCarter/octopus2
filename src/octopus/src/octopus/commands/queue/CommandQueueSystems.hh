#pragma once

#include "octopus/systems/phases/Phases.hh"

namespace octopus
{


template<typename variant_t>
void handle_action(CommandQueue<variant_t> &queue_p, CommandQueueActionDone const &action_p)
{
	queue_p._done = action_p._done;
}

template<typename variant_t>
void handle_action(CommandQueue<variant_t> &queue_p, CommandQueueActionReplace<variant_t> const &action_p)
{
	queue_p._queued = action_p._queued;
}

template<typename variant_t>
void handle_action(CommandQueue<variant_t> &queue_p, CommandQueueActionAddFront<variant_t> const &action_p)
{
	queue_p._queued.push_front(action_p._queued);
}

template<typename variant_t>
void handle_action(CommandQueue<variant_t> &queue_p, CommandQueueActionAddBack<variant_t> const &action_p)
{
	queue_p._queued.push_back(action_p._queued);
}

template<typename variant_t>
void set_up_command_queue_systems(flecs::world &ecs, CommandQueueMementoManager<variant_t> &mementoManager_p, StateStepContainer<variant_t> &stateStep_p)
{
	// set up relations
    CommandQueue<variant_t>::state(ecs).add(flecs::Exclusive);
    CommandQueue<variant_t>::cleanup(ecs).add(flecs::Exclusive);

	// Add memento
	ecs.system("CommandQueueMementoSetup")
		.kind(ecs.entity(InitializationPhase))
		.iter([&mementoManager_p](flecs::iter& it) {
			mementoManager_p.lMementos.push_back(typename CommandQueueMementoManager<variant_t>::vMemento());
		});

	// Apply actions
	ecs.system<CommandQueue<variant_t>>()
		.kind(ecs.entity(PrepingUpdatePhase))
		.each([&ecs, &mementoManager_p](flecs::entity e, CommandQueue<variant_t> &queue_p) {

			if(queue_p._queuedActions.size() > 0)
			{
				mementoManager_p.lMementos.back().push_back(memento(e, queue_p));
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
		.kind(ecs.entity(PrepingUpdatePhase))
		.write(CommandQueue<variant_t>::cleanup(ecs), flecs::Wildcard)
		.each([&ecs, &stateStep_p](flecs::entity e, CommandQueue<variant_t> &queue_p) {
			queue_p.clean_up_current(ecs, e, stateStep_p);
		});

	ecs.system<CommandQueue<variant_t>>()
		.kind(ecs.entity(PostCleanUpPhase))
		.write(CommandQueue<variant_t>::cleanup(ecs), flecs::Wildcard)
		.each([&ecs, &stateStep_p](flecs::entity e, CommandQueue<variant_t> &queue_p) {
			queue_p.update_current(ecs, e, stateStep_p);
		});
}

}
