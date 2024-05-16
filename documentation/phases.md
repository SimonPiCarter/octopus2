# Phases

Describe the flecs system phases and what happen in every one of them.

Here are all the default phases in the running order :

## OnLoad
Loading input

## PostLoad

Setting up from intput.

- CommandQueue : marking current has done and tagging clean up and removing state of entities

## PreUpdate

- Commands : all clean ups from commands

## OnUpdate

- CommandQueue : updating states based on queue

## OnValidate

- Commands : all actions from commands

## PostUpdate

- CommandQueue : register potential done step
- Step : apply all

## PreStore

Prepare display

- Validate components (do various checks and corrections)

## OnStore

Sync with display. This is the only time where display and backend must be synced

## Revert

Custom phase not run by default use to revert steps
