# Phases

Describe the flecs system phases and what happen in every one of them.

Here are all the default phases in the running order :

## PrepingUpdate

### CommandQueueSystems

- Push a new memento layer
- Take Queue Actions into account
- Remove current state and add clean up state if done  in entities

## CleanUp

### CommandQueueSystems

- Commands : all clean ups from commands

## PreUpdate

### CommandQueueSystems

- Remove clean up state and add new state if any and component associated

## Update

- all actions from commands

## PostUpdate

### Moves and position

- Update PositionStep from Move

## Stepping

- Step : apply all

## Validate

- Validate components (do various checks and corrections)
