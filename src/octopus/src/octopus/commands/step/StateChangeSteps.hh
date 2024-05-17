#pragma once

namespace octopus
{

template<typename variant_t>
struct StateAddPairStep
{
	flecs::entity ent;
	flecs::entity pair_first;
	variant_t pair_second;
	variant_t old_pair_second;
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

template<typename variant_t>
struct StateStepContainer
{
	std::list<std::vector<StateAddPairStep<variant_t>>> _addPair;
	std::list<std::vector<StateRemovePairStep<variant_t>>> _removePair;
	std::list<std::vector<StateSetComponentStep<variant_t>>> _setComp;

	void add_layer()
	{
		_addPair.push_back(std::vector<StateAddPairStep<variant_t>>());
		_removePair.push_back(std::vector<StateRemovePairStep<variant_t>>());
		_setComp.push_back(std::vector<StateSetComponentStep<variant_t>>());
	}

	void pop_layer()
	{
		_addPair.pop_front();
		_removePair.pop_front();
		_setComp.pop_front();
	}

	template<typename type_t>
	void add(flecs::entity e, flecs::entity state, type_t const &)
	{
		e.add_second<typename type_t::State>(state);
	}

	template<typename type_t>
	void remove(flecs::entity e, flecs::entity state, type_t const &)
	{
		e.remove_second<typename type_t::State>(state);
	}

	void apply(flecs::world &ecs)
	{
		for(StateAddPairStep<variant_t> const &step_l : _addPair.back())
		{
			std::visit([this, &step_l, &ecs](auto&& arg) { add(step_l.ent.mut(ecs), step_l.pair_first, arg); }, step_l.pair_second);
		}
		for(StateRemovePairStep<variant_t> const &step_l : _removePair.back())
		{
			std::visit([this, &step_l, &ecs](auto&& arg) { remove(step_l.ent.mut(ecs), step_l.pair_first, arg); }, step_l.pair_second);
		}
		for(StateSetComponentStep<variant_t> const &step_l : _setComp.back())
		{
			std::visit([this, &step_l, &ecs](auto&& arg) { step_l.ent.mut(ecs).set(arg); }, step_l.new_value);
		}
	}

	void revert(flecs::world &ecs)
	{
		for(auto rit_l = _addPair.back().rbegin() ; rit_l != _addPair.back().rend() ; ++ rit_l)
		{
			StateAddPairStep<variant_t> const &step_l = *rit_l;
			std::visit([this, &step_l, &ecs](auto&& arg) { add(step_l.ent.mut(ecs), step_l.pair_first, arg); }, step_l.old_pair_second);
		}
		for(auto rit_l = _removePair.back().rbegin() ; rit_l != _removePair.back().rend() ; ++ rit_l)
		{
			StateRemovePairStep<variant_t> const &step_l = *rit_l;
			std::visit([this, &step_l, &ecs](auto&& arg) { add(step_l.ent.mut(ecs), step_l.pair_first, arg); }, step_l.pair_second);
		}
		for(auto rit_l = _setComp.back().rbegin() ; rit_l != _setComp.back().rend() ; ++ rit_l)
		{
			StateSetComponentStep<variant_t> const &step_l = *rit_l;
			std::visit([this, &step_l, &ecs](auto&& arg) { step_l.ent.mut(ecs).set(arg); }, step_l.old_value);
		}
	}
};

} // namespace octopus
