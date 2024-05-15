
#pragma once

#include <flecs.h>
#include <vector>

namespace octopus
{

template<typename T>
struct StepTuple {
	typename flecs::ref<typename T::Data> data;
	typename T step;
	typename T::Memento memento;
};

template<typename T>
struct StepVector {
	typename std::vector<typename StepTuple<T> > steps;

	void add_step(flecs::entity ent, typename T && step_p)
	{
		steps.push_back({ent.get_ref<typename T::Data>(), step_p, T::Memento()});
	}

	void add_step(flecs::ref<typename T::Data> ref, typename T && step_p)
	{
		steps.push_back({ref, step_p, T::Memento()});
	}
};

template<typename T>
void apply_step(typename T::Memento &memento, typename T::Data &d, typename T const &s);

template<typename T>
void apply_step_tuple(typename StepTuple<T> &s)
{
	typename T::Data * d = s.data.try_get();
	if(d)
	{
		apply_step(s.memento, *d, s.step);
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
void revert_step(typename T::Data &d, typename T::Memento const &memento);

template<typename T>
void revert_step_tuple(typename StepTuple<T> &s)
{
	typename T::Data * d = s.data.try_get();
	if(d)
	{
		revert_step<T>(*d, s.memento);
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