#pragma once

#include <variant>

#include "octopus/commands/basic/move/AttackCommand.hh"
#include "octopus/components/advanced/production/queue/ProductionQueue.hh"
#include "octopus/components/basic/ability/Caster.hh"
#include "octopus/components/basic/attack/Attack.hh"
#include "octopus/components/basic/flock/Flock.hh"
#include "octopus/components/basic/hitpoint/Destroyable.hh"
#include "octopus/components/basic/hitpoint/HitPoint.hh"
#include "octopus/components/basic/hitpoint/HitPointMax.hh"
#include "octopus/components/basic/position/Move.hh"
#include "octopus/components/basic/position/Position.hh"
#include "octopus/components/basic/position/PositionInTree.hh"
#include "octopus/components/step/StepContainer.hh"
#include "octopus/world/resources/ResourceStock.hh"

#define DEFAULT_STEPS_T octopus::HitPointStep, \
octopus::HitPointMaxStep, \
octopus::DestroyableStep, \
octopus::PositionStep, \
octopus::PositionInTreeStep, \
octopus::MassStep, \
octopus::VelocityStep, \
octopus::CollisionStep, \
octopus::AttackWindupStep, \
octopus::AttackReloadStep, \
octopus::AttackCommandStep, \
octopus::AttackCommandInitStep, \
octopus::FlockArrivedStep, \
octopus::ProductionQueueTimestampStep, \
octopus::ProductionQueueOperationStep, \
octopus::ResourceStockStep, \
octopus::CasterLastCastStep, \
octopus::CasterWindupStep

namespace octopus
{

template<class... Ts>
struct StepManager
{
	typedef std::variant<Ts...> Variant;
	typedef StepContainerCascade<Ts...> StepContainer;

	std::list<std::vector<StepContainer> > steps;
	std::list<std::vector<StepContainer> > presteps;


	uint32_t steps_added = 0;

	void add_layer(size_t threads_p)
	{
		++steps_added;
		steps.push_back(std::vector<StepContainer>(threads_p, makeStepContainer<Ts...>()));
		presteps.push_back(std::vector<StepContainer>(threads_p, makeStepContainer<Ts...>()));
	}

	void pop_layer()
	{
		steps.pop_front();
		presteps.pop_front();
	}

	void pop_last_layer()
	{
		if(!steps.empty())
		{
			--steps_added;
			steps.pop_back();
			presteps.pop_back();
		}
	}

	std::vector<StepContainer> &get_last_layer()
	{
		return steps.back();
	}

	std::vector<StepContainer> &get_last_prelayer()
	{
		return presteps.back();
	}
};

using DefaultStepManager = StepManager<DEFAULT_STEPS_T>;

/// @brief Store all steps required to progress or go back into the states of the world
/// @tparam variant_t the command variant used by the CommandQueue
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
struct DefaultStepContext : StepContext<variant_t, DEFAULT_STEPS_T>
{};

template<typename variant_t>
StepContext<variant_t, DEFAULT_STEPS_T> makeDefaultStepContext()
{
	return StepContext<variant_t, DEFAULT_STEPS_T>();
}

template<typename variant_t, class... Ts>
StepContext<variant_t, DEFAULT_STEPS_T, Ts...> makeStepContext()
{
	return StepContext<variant_t, DEFAULT_STEPS_T, Ts...>();
}



} // namespace octopus

