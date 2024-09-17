#pragma once

#include "octopus/components/basic/flock/Flock.hh"
#include "octopus/components/basic/position/PositionInTree.hh"

namespace octopus
{

/// @brief Store all steps required to progress or go back into the states of the world
/// @tparam variant_t the command variant sued by the CommandQueue
/// @tparam StepManager_t An instanciation of StepManager (from StepContainer.hh)
/// @tparam StateStepManager_t An instanciation of StateStepContainer (from StateStepChange.hh)
template<typename variant_t, typename... steps_t>
struct StepContext
{
	typedef variant_t variant;
	typedef StepManager<steps_t...> step;
	CommandQueueMementoManager<variant_t> memento_manager;
	StepManager<steps_t...> step_manager;
	StateStepContainer<variant_t> state_step_manager;
};

#define DEFAULT_STEPS_T HitPointStep, \
HitPointMaxStep, \
PositionStep, \
PositionInTreeStep, \
MassStep, \
VelocityStep, \
CollisionStep, \
AttackWindupStep, \
AttackReloadStep, \
AttackCommandStep, \
FlockArrivedStep

template<typename variant_t>
struct DefaultStepContext : StepContext<variant_t, DEFAULT_STEPS_T>
{};

template<typename variant_t>
StepContext<variant_t, HitPointStep, DEFAULT_STEPS_T> makeDefaultStepContext()
{
	return StepContext<variant_t, HitPointStep, DEFAULT_STEPS_T>();
}

template<typename variant_t, class... Ts>
StepContext<variant_t, HitPointStep, DEFAULT_STEPS_T, Ts...> makeStepContext()
{
	return StepContext<variant_t, HitPointStep, DEFAULT_STEPS_T, Ts...>();
}



} // namespace octopus

