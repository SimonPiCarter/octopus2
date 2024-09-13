#include <gtest/gtest.h>

#include "flecs.h"


TEST(simple, simple)
{
	enum Movement {
		Walking,
		Running,
		None
	};

	enum Direction {
		Front,
		Back,
		Left,
		Right
	};

	flecs::world ecs;

    ecs.component<Movement>().add(flecs::Union);
    ecs.component<Direction>().add(flecs::Union);

    // Create a query that subscribes for all entities that have a Direction
    // and that are walking.
    // with<T>() requests no data by default, so we must specify what we want.
    // in() requests Read-Only
    flecs::query<> q = ecs.query_builder()
        .with(Walking).in()
        .with<Direction>(flecs::Wildcard).in()
        .build();

    // Create a few entities with various state combinations
    ecs.entity("e1")
        .add(Walking)
        .add(Front);

    ecs.entity("e2")
        .add(Running)
        .add(Left);

    flecs::entity e3 = ecs.entity("e3")
        .add(Running)
        .add(Back);

    // Add Walking to e3. This will remove the Running case
    e3.add(Walking);

    // Iterate the query
    q.run([&](flecs::iter& it) {
		while (it.next()) {
			// Get the column with direction states. This is stored as an array
			// with identifiers to the individual states
			auto movement = it.field<const flecs::entity_t>(1);
			auto direction = it.field<const flecs::entity_t>(2);

			for (auto i : it) {
				// Movement will always be Walking, Direction can be any state
				std::cout << it.entity(i).name()
					<< ": Movement: "
					<< it.world().get_alive(movement[i]).name()
					<< ", Direction: "
					<< it.world().get_alive(direction[i]).name()
					<< std::endl;
			}
		}
    });
}
