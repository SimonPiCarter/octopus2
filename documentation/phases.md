# Phases

Describe the flecs system phases and what happen in every one of them.

Here are all the default phases in the running order :

## Initialization (InitializationPhase)

### CommandQueueSystems

- Push a new memento layer

### Steps

- Push new step layer for commands actions (StepManager.get_last_layer)
- Push new step layer for command clean up actions (StepManager.get_last_prelayer)
- Push new step layer for state modifiction (StateChangeSteps) : modifying components

## PrepingUpdate (PrepingUpdatePhase)

### CommandQueueSystems

- Take Queue Actions into account
- Remove current state and add clean up state if done  in entities

## CleanUp (CleanUpPhase)

### CommandQueueSystems

- Commands : all clean ups from commands

## PostCleanUp (PostCleanUpPhase)

### CommandQueueSystems

- Remove clean up state and add new state if any and component associated

## PreUpdate (PreUpdatePhase)

Apply all steps :
- modifying state (StateChangeSteps.get_last_prelayer)
- clean up actions (StepManager.get_last_prelayer)

## Update (UpdatePhase)

### Position

- Update AABB Tree in PositionContext (PositionSystems)

### Hitpoints

- Update Destroyable from Hitpoints

### TimeStamp

- Update time stamp (using step so update will be shown on next iteration)

## PostUpdate (PostUpdatePhase)

### commands

- All actions from commands

### production

- All production

## EndUpdate

### attack resolution

- All attacks that have been triggered

## Moving (MovingPhase)

### Moves and position

- Collision algorithm
- Update PositionStep from Move
- Update VelocityStep from Move

## Input (InputPhase)

Handle all inputs and inject them into the world
- Production
- Commands actions

This has to be done just before stepping to avoid steps from input to be overriden by
running commands or productions.

## Stepping (SteppingPhase)

- modifying state (StateChangeSteps.get_last_layer)
- Step : apply all (StepManager.get_last_layer)

## Validate (ValidatePhase)

- Validate components (do various checks and corrections)

- Synchronization of Triangulation should happen here

## DisplaySync (DisplaySyncPhase)

- sync with display

## EndCleanup (EndCleanUpPhase)

- destroy entities

### attack resolution

- remove AttackTrigger