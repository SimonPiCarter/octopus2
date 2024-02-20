
#pragma once

#include <flecs.h>
#include <vector>

namespace octopus
{

template<typename T>
void apply_step(T &m, typename T::Data &d, typename T::Step const &s);

template<typename T>
void revert_memento(typename T::Data &d, T const &m);

template<typename T>
struct StepPair {
	typename flecs::ref<typename T::Data> data;
	typename T::Step step;
	typename T memento;
};

template<typename T>
struct StepVector {
	typename std::vector<typename StepPair<T> > steps;

	void add_step(flecs::entity ent, typename T::Step && step_p)
	{
		steps.push_back({ent.get_ref<typename T::Data>(), step_p, T()});
	}

	void add_step(flecs::ref<typename T::Data> ref, typename T::Step && step_p)
	{
		steps.push_back({ref, step_p, T()});
	}
};


template<typename T>
void apply_step(typename StepPair<T> &s)
{
	typename T::Data * d = s.data.try_get();
	if(d)
	{
		apply_step(s.memento, *d, s.step);
	}
}

template<typename T>
void revert_memento(typename StepPair<T> &s)
{
	typename T::Data * d = s.data.try_get();
	if(d)
	{
		revert_memento(*d, s.memento);
	}
}

template<typename T>
void apply(StepVector<T> &vec, size_t i)
{
	apply_step<T>(vec.steps[i]);
}

template<typename T>
void revert(StepVector<T> &vec, size_t i)
{
	revert_memento<T>(vec.steps[i]);
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
void revert_all(StepVector<T> &vec)
{
	for(size_t i = vec.steps.size() ; i > 0 ; -- i)
	{
		revert(vec, i-1);
	}
}

} // octopus
