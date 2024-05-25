#pragma once

#include <vector>
#include <variant>
#include "flecs.h"

#include "Step.hh"

#include "octopus/components/basic/attack/Attack.hh"
#include "octopus/components/basic/hitpoint/HitPoint.hh"
#include "octopus/components/basic/hitpoint/HitPointMax.hh"
#include "octopus/components/basic/position/Position.hh"
#include "octopus/commands/basic/move/AttackCommand.hh"

#include "octopus/utils/ThreadPool.hh"

namespace octopus
{

template <class... Ts>
struct StepContainerCascade {};

template <class T, class... Ts>
struct StepContainerCascade<T, Ts...> : StepContainerCascade<Ts...> {
	StepContainerCascade(T t, Ts... ts) : StepContainerCascade<Ts...>(ts...) {}

	template <class G>
	typename std::enable_if<!std::is_same<G, T>::value, StepVector<G> &>::type get() {
		// cascade
		StepContainerCascade<Ts...>& base = *this;
		return base.get<G>();
	}

	template <class G>
	typename std::enable_if<std::is_same<G, T>::value, StepVector<G> &>::type get() {
		return steps;
	}

	StepVector<T> steps;
};

template <class... Ts>
StepContainerCascade<Ts...> makeStepContainer()
{
	return StepContainerCascade<Ts...>(Ts()...);
}

template<class... Ts> void reserve(StepContainerCascade<Ts...> &container, size_t size) {}
template<class T, class... Ts> void reserve(StepContainerCascade<T, Ts...> &container, size_t size)
{
	container.steps.steps.resize(size);
	// cascade
	StepContainerCascade<Ts...>& base = container;
	reserve(base, size);
}

template<class... Ts> void clear_container(StepContainerCascade<Ts...> &container) {}
template<class T, class... Ts> void clear_container(StepContainerCascade<T, Ts...> &container)
{
	container.steps.steps.clear();
	// cascade
	StepContainerCascade<Ts...>& base = container;
	clear_container(base);
}

template<class... Ts> void apply_container(StepContainerCascade<Ts...> &container, std::vector<std::function<void()>> &jobs) {}
template<class T, class... Ts> void apply_container(StepContainerCascade<T, Ts...> &container, std::vector<std::function<void()>> &jobs)
{
	jobs.push_back([&container]() {
			apply_all(container.steps);
	});

	// cascade
	StepContainerCascade<Ts...>& base = container;
	apply_container(base, jobs);
}

template<class... Ts> void revert_container(StepContainerCascade<Ts...> &container, std::vector<std::function<void()>> &jobs) {}
template<class T, class... Ts> void revert_container(StepContainerCascade<T, Ts...> &container, std::vector<std::function<void()>> &jobs)
{
	jobs.push_back([&container]() {
			revert_all(container.steps);
	});

	// cascade
	StepContainerCascade<Ts...>& base = container;
	revert_container(base, jobs);
}

template<class StepContainer_t>
void dispatch_apply(std::vector<StepContainer_t> &container, ThreadPool &pool)
{
	for(size_t i = 0 ; i < container.size(); ++ i)
	{
		std::vector<std::function<void()>> jobs_l;

		apply_container(container[i], jobs_l);

		enqueue_and_wait(pool, jobs_l);
	}
}

template<class StepContainer_t>
void dispatch_revert(std::vector<StepContainer_t> &container, ThreadPool &pool)
{
	for(size_t i = container.size() ; i > 0; -- i)
	{
		std::vector<std::function<void()>> jobs_l;

		revert_container(container[i-1], jobs_l);

		enqueue_and_wait(pool, jobs_l);
	}
}

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

} // octopus
