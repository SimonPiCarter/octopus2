# Phases

Describe the flecs system phases and what happen in every one of them.

Here are all the default phases in the running order :

## Initialization

### CommandQueueSystems

- Push a new memento layer

### Steps

- Push new step layer for commands actions (StepManager.get_last_layer)
- Push new step layer for command clean up actions (StepManager.get_last_prelayer)
- Push new step layer for state modifiction (StateChangeSteps) : modifying components

## PrepingUpdate

### CommandQueueSystems

- Take Queue Actions into account
- Remove current state and add clean up state if done  in entities

## CleanUp

### CommandQueueSystems

- Commands : all clean ups from commands

## PostCleanUp

### CommandQueueSystems

- Remove clean up state and add new state if any and component associated

## PreUpdate

Apply all steps :
- modifying state (StateChangeSteps.get_last_prelayer)
- clean up actions (StepManager.get_last_prelayer)

## Update

- all actions from commands

## PostUpdate

### Moves and position

- Update PositionStep from Move

## Stepping

- modifying state (StateChangeSteps.get_last_layer)
- Step : apply all (StepManager.get_last_layer)

## Validate

- Validate components (do various checks and corrections)

## DisplaySync

- sync with display
