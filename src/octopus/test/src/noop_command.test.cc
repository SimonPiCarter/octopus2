#include <gtest/gtest.h>

#include "octopus/commands/basic/NoOpCommand.hh"
#include "octopus/commands/queue/CommandQueue.hh"
#include "octopus/systems/Systems.hh"
#include "octopus/world/WorldContext.hh"
#include "octopus/world/StepContext.hh"
#include "octopus/components/basic/position/Position.hh" // Included for basic entity setup

// Define a specific CommandQueue type for NoOpCommand if it doesn't use a variant
// For simplicity, assuming NoOpCommand might be part of a variant queue or handled by default.
// If NoOpCommand needs specific systems, those would need to be set up.
// Based on NoOpCommand.hh, it seems simple enough to be handled by the default queue logic.

using namespace octopus;

// Using a simple CommandQueue that can hold NoOpCommands.
// If tests with mixed command types are needed, a std::variant would be used.
using NoOpCommandQueue = CommandQueue<NoOpCommand>;

TEST(NoOpCommandTest, SimpleNoOp)
{
    WorldContext world_l;
    world_l.ecs.component<Position>(); // Ensure Position is registered for entity creation
    world_l.ecs.component<NoOpCommandQueue>(); // Register our command queue component

    // Register NoOpCommand and its state if necessary (often done globally or by setup systems)
    // NoOpCommand itself is simple, its State is empty.
    // The main thing is that the CommandQueue system knows about NoOpCommand::State.
    // This is typically handled by advanced_components_support or similar in a real setup,
    // but for a minimal test, we rely on set_up_systems to handle basic command processing.
    world_l.ecs.component<NoOpCommand::State>();


    StepContext stepContext_l = makeDefaultStepContext<NoOpCommand>();

    // Basic systems setup
    // This should include command queue processing systems.
    set_up_systems(world_l, stepContext_l);

    flecs::world &ecs_l = world_l.ecs;

    // Create an entity
    auto entity_l = ecs_l.entity()
        .add<Position>() // Add some basic component
        .add<NoOpCommandQueue>();

    // Check initial queue state (should be empty)
    const auto * cq_initial_l = entity_l.get<NoOpCommandQueue>();
    ASSERT_NE(nullptr, cq_initial_l);
    ASSERT_TRUE(cq_initial_l->hasCommand()); // Initially has the implicit "no command"
    ASSERT_TRUE(cq_initial_l->_futureQueuedActions.empty());
    ASSERT_TRUE(cq_initial_l->_queuedActions.empty());
    ASSERT_EQ(0u, cq_initial_l->getCurrentCommandId());


    // Add a NoOpCommand
    NoOpCommandQueue* cq_mut_l = entity_l.get_mut<NoOpCommandQueue>();
    ASSERT_NE(nullptr, cq_mut_l);
    cq_mut_l->_queuedActions.push_back(CommandQueueActionAddBack<NoOpCommand>{NoOpCommand{}});

    // Progress the world to process the command
    ecs_l.progress();

    // Assertions after processing
    const auto * cq_after_l = entity_l.get<NoOpCommandQueue>();
    ASSERT_NE(nullptr, cq_after_l);

    // The NoOpCommand should be processed and removed.
    // The queue should be back to its "idle" state.
    // Depending on exact CommandQueue logic, "done" might mean empty _queuedActions
    // and the internal state reflects completion.
    // A NoOp command is typically consumed in one step and marked done.
    ASSERT_TRUE(cq_after_l->_queuedActions.empty()); // Explicit actions should be gone
                                                    // Check if it has reverted to the implicit "no command" state
                                                    // or if currentCommandId reflects the processed command.
                                                    // For NoOp, it should be fully consumed.
    ASSERT_TRUE(cq_after_l->isLastCommandDone());

    // Ensure no other state (like position) changed unexpectedly
    const Position *pos_l = entity_l.get<Position>();
    ASSERT_NE(nullptr, pos_l);
    ASSERT_EQ(0., pos_l->pos.x.to_double()); // Assuming default Position is (0,0)
    ASSERT_EQ(0., pos_l->pos.y.to_double());
}
