
#pragma once

#include <flecs.h>
#include <vector>

namespace octopus
{

template<typename T>
struct StepTuple {
	flecs::ref<typename T::Data> data;
	T step;
	typename T::Memento memento;
};

template<typename T>
struct StepVector {
	std::vector<StepTuple<T> > steps;

	void add_step(flecs::entity ent, T && step_p)
	{
		steps.push_back({ent.get_ref<typename T::Data>(), step_p, typename T::Memento()});
	}

	void add_step(flecs::ref<typename T::Data> ref, T && step_p)
	{
		steps.push_back({ref, step_p, T::Memento()});
	}
};

template<typename T>
void apply_step_tuple(StepTuple<T> &s)
{
	typename T::Data * d = s.data.try_get();
	if(d)
	{
		s.step.apply_step(*d, s.memento);
	}
}

template<typename T>
void apply(StepVector<T> &vec, size_t i)
{
	apply_step_tuple<T>(vec.steps[i]);
}

template<typename T>
void apply_all(StepVector<T> &vec)
{
	for(size_t i = 0 ; i < vec.steps.size() ; ++ i)
	{
		apply(vec, i);
	}
}

template<typename T>
void revert_step_tuple(StepTuple<T> &s)
{
	typename T::Data * d = s.data.try_get();
	if(d)
	{
		s.step.revert_step(*d, s.memento);
	}
}

template<typename T>
void revert(StepVector<T> &vec, size_t i)
{
	revert_step_tuple<T>(vec.steps[i]);
}

template<typename T>
void revert_all(StepVector<T> &vec)
{
	for(size_t i = vec.steps.size() ; i > 0 ; -- i)
	{
		revert(vec, i-1);
	}
}

} // octopus
