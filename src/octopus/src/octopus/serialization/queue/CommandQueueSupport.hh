#pragma once

#include "flecs.h"
#include <list>

#include "octopus/serialization/containers/ListSupport.hh"
#include "octopus/serialization/variant/VariantSupport.hh"

#include "octopus/commands/queue/CommandQueue.hh"
#include "octopus/commands/queue/action/CommandQueueAction.hh"

namespace octopus
{

// Reusable reflection support for std::vector
template<typename... tArgs>
void command_queue_support(flecs::world& world) {

    world.component<octopus::NoOpCommand>()
		.member<uint32_t>("no_op");

	variant_support<tArgs...>(world);

	typedef std::variant<tArgs...> variant_args;

    world.component<std::list<variant_args> >()
        .opaque(std_list_support<variant_args>);

	////
	//// Actions
	////

    world.component<octopus::CommandQueueActionDone>()
		.member<bool>("done");

    world.component<octopus::CommandQueueActionReplace<variant_args> >()
		.member("queued", &octopus::CommandQueueActionReplace<variant_args>::_queued);

    world.component<octopus::CommandQueueActionAddFront<variant_args> >()
		.member("queued", &octopus::CommandQueueActionAddFront<variant_args>::_queued);

    world.component<octopus::CommandQueueActionAddBack<variant_args> >()
		.member("queued", &octopus::CommandQueueActionAddBack<variant_args>::_queued);

	variant_support<
		octopus::CommandQueueActionDone,
		octopus::CommandQueueActionReplace<variant_args >,
		octopus::CommandQueueActionAddFront<variant_args >,
		octopus::CommandQueueActionAddBack<variant_args > >(world);

	typedef std::variant<
		octopus::CommandQueueActionDone,
		octopus::CommandQueueActionReplace<variant_args >,
		octopus::CommandQueueActionAddFront<variant_args >,
		octopus::CommandQueueActionAddBack<variant_args >
	> variant_actions;

    world.component<std::list<variant_actions> >()
        .opaque(std_list_support<variant_actions>);

    world.component<octopus::CommandQueue<variant_args> >()
        .member("current", &octopus::CommandQueue<variant_args>::_current)
        .member("queued", &octopus::CommandQueue<variant_args>::_queued)
		.member("done", &octopus::CommandQueue<variant_args>::_done)
        .member("queued_action", &octopus::CommandQueue<variant_args>::_queuedActions);
}

}