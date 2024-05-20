#pragma

#include "flecs.h"
#include "octopus/utils/ThreadPool.hh"
#include "octopus/components/step/StepContainer.hh"
#include "octopus/commands/queue/CommandQueue.hh"

namespace octopus
{

/// @brief revert a given number of steps and command memento
/// @note do no clear the containers
/// @tparam StepManager_t instance of StepManager in StepContainer.hh
/// @tparam CommandMementoManager_t CommandQueueMementoManager in CommandQueue.hh
/// @tparam StateStepContainer_t StateStepContainer in StateChangeStep.hh
/// @param steps_p number of steps to revert
/// @param step_manager_p
/// @param command_memento_p
/// @param state_step_container_p
template<class StepManager_t, class CommandMementoManager_t, class StateStepContainer_t>
void revert_n_steps(flecs::world &ecs, ThreadPool &pool_p, size_t steps_p,
	StepManager_t &step_manager_p,
	CommandMementoManager_t const &command_memento_p,
	StateStepContainer_t & state_step_container_p
)
{
	flecs::query<CommandQueue<typename CommandMementoManager_t::variant>> q = ecs.query<CommandQueue<typename CommandMementoManager_t::variant>>();

	size_t memento_reverted = 0;
	for(auto rit_l = command_memento_p.lMementos.rbegin() ; memento_reverted < steps_p && rit_l != command_memento_p.lMementos.rend() ; ++ rit_l)
	{
		// clear queued actions first
		q.each([](flecs::entity e, CommandQueue<typename CommandMementoManager_t::variant> &queue_p) {
			queue_p._queuedActions.clear();
		});
		// apply all mementos
		std::vector<CommandQueueMemento<custom_variant> > const &mementos_l = *rit_l;
		for(CommandQueueMemento<custom_variant> const &memento_l : mementos_l)
		{
			restore(ecs, memento_l);
		}

		++memento_reverted;
	}

	// revert component addition/deletion
	size_t steps_state_reverted = 0;
	for(auto rit_l = state_step_container_p.layers.rbegin() ; steps_state_reverted < steps_p && rit_l != state_step_container_p.layers.rend() ; ++ rit_l)
	{
		rit_l->revert(ecs);
		++steps_state_reverted;
	}

	// revert components (steps)
	size_t steps_reverted = 0;
	for(auto rit_l = step_manager_p.steps.rbegin() ; steps_reverted < steps_p && rit_l != step_manager_p.steps.rend() ; ++ rit_l)
	{
		std::vector<typename StepManager_t::StepContainer> &steps_l = *rit_l;
		dispatch_revert(steps_l, pool_p);
		++steps_reverted;
	}

	// revert component addition/deletion (presteps)
	size_t presteps_state_reverted = 0;
	for(auto rit_l = state_step_container_p.prelayers.rbegin() ; presteps_state_reverted < steps_p && rit_l != state_step_container_p.prelayers.rend() ; ++ rit_l)
	{
		rit_l->revert(ecs);
		++presteps_state_reverted;
	}

	// revert components (presteps)
	size_t presteps_reverted = 0;
	for(auto rit_l = step_manager_p.presteps.rbegin() ; presteps_reverted < steps_p && rit_l != step_manager_p.presteps.rend() ; ++ rit_l)
	{
		std::vector<typename StepManager_t::StepContainer> &steps_l = *rit_l;
		dispatch_revert(steps_l, pool_p);
		++presteps_reverted;
	}

	// sanity check
	if(steps_reverted != memento_reverted)
	{
		throw std::logic_error("Tried to revert in an incoherent state, not the same number of reverted steps for steps and command mementos");
	}
}

/// @brief Remove the steps_p latest steps from components and commands
/// @tparam StepManager_t instance of StepManager in StepContainer.hh
/// @tparam CommandMementoManager_t CommandQueueMementoManager in CommandQueue.hh
/// @tparam StateStepContainer_t StateStepContainer in StateChangeStep.hh
/// @param steps_p
/// @param step_manager_p
/// @param command_memento_p
template<class StepManager_t, class CommandMementoManager_t, class StateStepContainer_t>
void clear_n_steps(size_t steps_p, StepManager_t &step_manager_p,
	CommandMementoManager_t &command_memento_p,
	StateStepContainer_t & state_step_container_p)
{
	for(size_t i = 0 ; i < steps_p; ++i)
	{
		step_manager_p.pop_last_layer();
		state_step_container_p.pop_last_layer();
		if(!command_memento_p.lMementos.empty())
			command_memento_p.lMementos.pop_back();
	}
}

} // namespace octopus
