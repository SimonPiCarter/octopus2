#include <gtest/gtest.h>

#include "flecs.h"

TEST(state_extension, simple)
{
	// Basic state system
	enum class State {
		Walk,
		Run,
		None
	};


	enum class StateExtension {
		Attack,
		None
	};

	flecs::world ecs;
    ecs.component<State>().add(flecs::Union);
    ecs.component<StateExtension>().add(flecs::Union);

    ////// Systems //////

	// Walk
	ecs.system<>()
		.with(State::Walk).in()
		.iter([](flecs::iter& it) {
			for (size_t i = 0; i < it.count(); i ++) {
				std::cout << it.entity(i).name() << " Walk " << std::endl;
			}
		});

	// Attack
	ecs.system<>()
		.with(StateExtension::Attack)
		.iter([](flecs::iter& it) {
			for (size_t i = 0; i < it.count(); i ++) {
				std::cout << it.entity(i).name() << " Attack " << std::endl;
			}
		});

    //  ENTITIES
    ecs.entity("e1")
        .add(State::Walk);

    ecs.entity("e2")
        .add(State::Run);

    flecs::entity e3 = ecs.entity("e3")
        .add(State::Run);

    e3.add(State::Walk);
    e3.add(StateExtension::Attack);

	ecs.progress();
}
