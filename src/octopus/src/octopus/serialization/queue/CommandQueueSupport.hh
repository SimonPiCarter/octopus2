#pragma once

#include "flecs.h"

#include "octopus/serialization/containers/ListSupport.hh"
#include "octopus/serialization/variant/VariantSupport.hh"

#include "octopus/commands/queue/CommandQueue.hh"

// Reusable reflection support for std::vector
template<typename... tArgs>
void command_queue_support(flecs::world& world) {

    world.component<octopus::NoOpCommand>()
		.member<uint32_t>("no_op");

	variant_support<tArgs...>(world);

    world.component<std::list<std::variant<tArgs...> > >()
        .opaque(std_list_support<std::variant<tArgs...> >);


    world.component<octopus::CommandQueue<std::variant<tArgs...>> >()
        .member<std::variant<tArgs...>>("current")
        .member<std::list<std::variant<tArgs...>>>("queued")
		.member<bool>("done");
}
