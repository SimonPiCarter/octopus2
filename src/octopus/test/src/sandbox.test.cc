#include <gtest/gtest.h>

#include "flecs.h"
#include <iostream>
#include <string>
#include <vector>
#include <variant>

#include "octopus/systems/phases/Phases.hh"
#include "octopus/components/step/StepContainer.hh"

using namespace octopus;

// This application demonstrates how to use custom phases for systems. The
// default pipeline will automatically run systems for custom phases as long as
// they have the flecs::Phase tag.

// Dummy system
void Sys(flecs::iter& it) {
    std::cout << "system " << it.system().name() << "\n";
}

TEST(sandbox, test)
{
	flecs::world ecs;
    // Create two custom phases that branch off of EcsOnUpdate. Note that the
    // phases have the Phase tag, which is necessary for the builtin pipeline
    // to discover which systems it should run.
    flecs::entity Physics = ecs.entity()
        .add(flecs::Phase)
        .depends_on(flecs::OnUpdate);

    flecs::entity Collisions = ecs.entity()
        .add(flecs::Phase)
        .depends_on(Physics);

    // Create 3 dummy systems.
    ecs.system("CollisionSystem")
        .kind(Collisions)
        .iter(Sys);

    ecs.system("PhysicsSystem")
        .kind(Physics)
        .iter(Sys);

    ecs.system("GameSystem")
        .kind(ecs.entity(UpdatePhase))
        .iter(Sys);

    // Run pipeline
    ecs.progress();

    // Output
    //   system GameSystem
    //   system PhysicsSystem
    //   system CollisionSystem
}
