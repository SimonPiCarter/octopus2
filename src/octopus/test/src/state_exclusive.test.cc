#include <gtest/gtest.h>

#include "flecs.h"


TEST(state_exclusive, simple)
{
	flecs::world ecs;
    flecs::entity state = ecs.entity().add(flecs::Exclusive);
	// Basic state system
    flecs::entity walk = ecs.entity();
    flecs::entity run = ecs.entity();

    auto e1 = ecs.entity("e1")
        .add(state, walk);
    auto e2 = ecs.entity("e2")
        .add(state, run);
    auto e3 = ecs.entity("e3")
        .add(state, run);

	// System
	ecs.system<>()
		.with(state, walk)
		.iter([](flecs::iter& it) {
			for (size_t i = 0; i < it.count(); i ++) {
				std::cout << it.entity(i).name() << " walking. " << std::endl;
			}
		});
	ecs.system<>()
		.with(state, run)
		.iter([](flecs::iter& it) {
			for (size_t i = 0; i < it.count(); i ++) {
				std::cout << it.entity(i).name() << " running. " << std::endl;
			}
		});


	ecs.progress();

	// extension
    flecs::entity attack = ecs.entity();
	ecs.system<>()
		.with(state, attack)
		.iter([](flecs::iter& it) {
			for (size_t i = 0; i < it.count(); i ++) {
				std::cout << it.entity(i).name() << " attacking. " << std::endl;
			}
		});

	e3.add(state, attack);

	ecs.progress();
}
