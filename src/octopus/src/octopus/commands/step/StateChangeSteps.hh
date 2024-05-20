#pragma once

#include "flecs.h"
#include <list>
#include <vector>

namespace octopus
{

template<typename variant_t>
struct StateAddPairStep
{
	flecs::entity ent;
	flecs::entity pair_first;
	variant_t pair_second;
};

template<typename variant_t>
struct StateRemovePairStep
{
	flecs::entity ent;
	flecs::entity pair_first;
	variant_t pair_second;
};

template<typename variant_t>
struct StateSetComponentStep
{
	flecs::entity ent;
	variant_t new_value;
	variant_t old_value;
};

template<typename type_t>
void add_second(flecs::entity e, flecs::entity state, type_t const &)
{
	e.add_second<typename type_t::State>(state);
}

template<typename type_t>
void remove_second(flecs::entity e, flecs::entity state, type_t const &)
{
	e.remove_second<typename type_t::State>(state);
}

template<typename variant_t>
struct StateStepLayer
{
	std::vector<StateAddPairStep<variant_t> > _addPair;
	std::vector<StateRemovePairStep<variant_t> > _removePair;
	std::vector<StateSetComponentStep<variant_t> > _setComp;


	void apply(flecs::world &ecs)
	{
		for(StateRemovePairStep<variant_t> const &step_l : _removePair)
		{
			std::visit([&step_l, &ecs](auto&& arg) { remove_second(step_l.ent.mut(ecs), step_l.pair_first, arg); }, step_l.pair_second);
		}
		for(StateAddPairStep<variant_t> const &step_l : _addPair)
		{
			std::visit([&step_l, &ecs](auto&& arg) { add_second(step_l.ent.mut(ecs), step_l.pair_first, arg); }, step_l.pair_second);
		}
		for(StateSetComponentStep<variant_t> const &step_l : _setComp)
		{
			std::visit([&step_l, &ecs](auto&& arg) { step_l.ent.mut(ecs).set(arg); }, step_l.new_value);
		}
	}

	void revert(flecs::world &ecs)
	{
		for(auto rit_l = _setComp.rbegin() ; rit_l != _setComp.rend() ; ++ rit_l)
		{
			StateSetComponentStep<variant_t> const &step_l = *rit_l;
			std::visit([this, &step_l, &ecs](auto&& arg) { step_l.ent.mut(ecs).set(arg); }, step_l.old_value);
		}
		for(auto rit_l = _addPair.rbegin() ; rit_l != _addPair.rend() ; ++ rit_l)
		{
			StateAddPairStep<variant_t> const &step_l = *rit_l;
			std::visit([this, &step_l, &ecs](auto&& arg) { remove_second(step_l.ent.mut(ecs), step_l.pair_first, arg); }, step_l.pair_second);
		}
		for(auto rit_l = _removePair.rbegin() ; rit_l != _removePair.rend() ; ++ rit_l)
		{
			StateRemovePairStep<variant_t> const &step_l = *rit_l;
			std::visit([this, &step_l, &ecs](auto&& arg) { add_second(step_l.ent.mut(ecs), step_l.pair_first, arg); }, step_l.pair_second);
		}
	}
};

template<typename variant_t>
struct StateStepContainer
{
	typedef variant_t Variant;
	std::list<StateStepLayer<variant_t> > layers;
	std::list<StateStepLayer<variant_t> > prelayers;

	void add_layer()
	{
		layers.push_back(StateStepLayer<variant_t>());
		prelayers.push_back(StateStepLayer<variant_t>());
	}

	void pop_layer()
	{
		if(!layers.empty())
		{
			layers.pop_front();
		}
		if(!prelayers.empty())
		{
			prelayers.pop_front();
		}
	}

	void pop_last_layer()
	{
		if(!layers.empty())
		{
			layers.pop_back();
		}
		if(!prelayers.empty())
		{
			prelayers.pop_back();
		}
	}

	StateStepLayer<variant_t> & get_last_layer()
	{
		return layers.back();
	}

	StateStepLayer<variant_t> & get_last_prelayer()
	{
		return prelayers.back();
	}
};

} // namespace octopus
