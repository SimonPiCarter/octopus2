#pragma once

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

template<typename variant_t>
StepContext<variant_t, HitPointStep, HitPointMaxStep, PositionStep, AttackWindupStep, AttackReloadStep, AttackCommandStep> makeDefaultStepContext()
{
	return StepContext<variant_t, HitPointStep, HitPointMaxStep, PositionStep, AttackWindupStep, AttackReloadStep, AttackCommandStep>();
}

template<typename variant_t, class... Ts>
StepContext<variant_t, HitPointStep, HitPointMaxStep, PositionStep, AttackWindupStep, AttackReloadStep, AttackCommandStep, Ts...> makeStepContext()
{
	return StepContext<variant_t, HitPointStep, HitPointMaxStep, PositionStep, AttackWindupStep, AttackReloadStep, AttackCommandStep, Ts...>();
}



} // namespace octopus

