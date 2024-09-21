#include <gtest/gtest.h>

#include "flecs.h"
#include <iostream>
#include <string>
#include <vector>
#include <variant>

#include "octopus/systems/phases/Phases.hh"
#include "octopus/components/step/StepContainer.hh"

using namespace octopus;

TEST(DISABLED_sandbox, test)
{
	struct Position {
		double x, y;
	};

	// Create tag type to use as event (could also use entity)
	struct MyEvent { };
	flecs::world ecs;

    // Create observer for custom event
    // ecs.observer<Position>()
    //     .event<MyEvent>()
    //     .each([](flecs::iter& it, size_t i, Position&) {
    //         std::cout << " - " << it.event().name() << ": "
    //             << it.event_id().str() << ": "
    //             << it.entity(i).name() << "\n";
    //     });
    ecs.observer<Position>()
        .event<MyEvent>()
        .each([](flecs::entity e, Position&) {
            std::cout << " - " << e.name() << "\n";
        });

    // The observer query can be matched against the entity, so make sure it
    // has the Position component before emitting the event. This does not
    // trigger the observer yet.
    flecs::entity e = ecs.entity("e")
        .set<Position>({10, 20});

	ecs.system<Position>()
        .each([&ecs](flecs::entity e, Position const &p) {
			// Emit the custom event
			ecs.event<MyEvent>()
				.id<Position>()
				.entity(e)
				.emit();
        });

	ecs.progress();
    // Output
    //   - MyEvent: Position: e
}
