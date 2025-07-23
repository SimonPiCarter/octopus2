#pragma once

#include <list>
#include <variant>

#include "flecs.h"

#include "octopus/commands/step/StateChangeSteps.hh"
#include "octopus/utils/log/Logger.hh"
#include "action/CommandQueueAction.hh"

namespace octopus
{

// System running commands should be part of the Update phase
// System cleaning up commands should be part of the CleanUp phase

template<typename type_t>
void add(flecs::entity &e, flecs::entity state, type_t const &)
{
	e.add_second<typename type_t::State>(state);
}

template<typename type_t>
void remove(flecs::entity &e, flecs::entity state, type_t const &)
{
	e.remove_second<typename type_t::State>(state);
}

/// @brief Memento used to store the state of the queue
template<typename variant_t>
struct CommandQueueMemento
{
	typedef std::variant<
		CommandQueueActionDone,
		CommandQueueActionReplace<variant_t>,
		CommandQueueActionAddFront<variant_t>,
		CommandQueueActionAddBack<variant_t>
	> CommandQueueAction;

	variant_t _current;
	std::list<variant_t> _queued;
	std::list<CommandQueueAction> _queuedActions;
	bool _done;
	flecs::entity e;
};

template<typename variant_t>
struct CommandQueueMementoManager
{
	typedef variant_t variant;
	typedef std::vector<CommandQueueMemento<variant_t> > vMemento;

	std::list<vMemento> lMementos;
};

template<typename variant_t>
struct CommandQueue
{
	typedef variant_t variant;
	/// @warning this is just an out of date pointer
	/// to get the type information of the current action
	variant_t _current;
	std::list<variant_t> _queued;
	/// @brief true if the current command is done
	bool _done = false;

	typedef std::variant<
		CommandQueueActionDone,
		CommandQueueActionReplace<variant_t>,
		CommandQueueActionAddFront<variant_t>,
		CommandQueueActionAddBack<variant_t>
	> CommandQueueAction;

	std::list<CommandQueueAction> _queuedActions;

	// PrepingUpdatePhase (1)
	// set clean up state
	void clean_up_current(flecs::world &ecs, flecs::entity &e, StateStepContainer<variant_t> &stateStep_p)
	{
		if(_done && !std::holds_alternative<NoOpCommand>(_current))
		{
			Logger::getDebug()<<"clean_up_current : resetting state to nothing"<<std::endl;
			// reset state
			stateStep_p.get_last_prelayer()._removePair.push_back({e, state(ecs), _current});
			// add clean up (do not use step here since at the end of the iteration this will be cleaned up)
			std::visit([this, &ecs, &e](auto&& arg) { add(e, cleanup(ecs), arg); }, _current);
			// reset variant
			_current = NoOpCommand();
		}
		// reset done
		_done = false;
	}

	template<typename type_t, typename StateStepLayer_t>
	void update_comp(flecs::entity &e, StateStepLayer_t &state_layer_p, type_t const &new_comp)
	{
		Logger::getDebug()<<"update_comp : setting comp "<<e.id()<<" "<<type_t::naming()<<std::endl;
		variant_t old_comp;
		type_t const * old_typped_value = e.try_get<type_t>();
		if(old_typped_value)
		{
			old_comp = *old_typped_value;
		}
		state_layer_p._setComp.push_back({e, new_comp, old_comp});
	}

	// PostCleanUpPhase (1)
	void update_current(flecs::world &ecs, flecs::entity &e, StateStepContainer<variant_t> &stateStep_p)
	{
		e.remove(cleanup(ecs), flecs::Wildcard);
		if(std::holds_alternative<NoOpCommand>(_current))
		{
			if(_queued.empty())
			{
				Logger::getDebug()<<"update_current : setting comp "<<e.id()<<" "<<NoOpCommand::naming()<<std::endl;
				// set state
				stateStep_p.get_last_prelayer()._addPair.push_back({e, state(ecs), NoOpCommand()});
			}
			else
			{
				_current = _queued.front();
				_queued.pop_front();

				Logger::getDebug()<<"update_current : setting comp "<<e.id()<<" front"<<std::endl;
				// set state
				stateStep_p.get_last_prelayer()._addPair.push_back({e, state(ecs), _current});

				// set component
				std::visit([this, &e, &stateStep_p](auto&& arg) { update_comp(e, stateStep_p.get_last_prelayer(), arg); }, _current);
			}
		}
	}

	static flecs::entity state(flecs::world &ecs) { return ecs.entity("state"); }
	static flecs::entity cleanup(flecs::world &ecs) { return ecs.entity("cleanup"); }
};

template<typename variant_t>
CommandQueueMemento<variant_t> memento(flecs::entity const &e, CommandQueue<variant_t> const &queue_p)
{
	return {
		queue_p._current,
		queue_p._queued,
		queue_p._queuedActions,
		queue_p._done,
		e
	};
}

template<typename queue_t>
queue_t * get_queue(flecs::entity e, queue_t const &)
{
	return e.try_get_mut<queue_t>();
}

template<typename variant_t>
void restore(flecs::world &ecs, CommandQueueMemento<variant_t> const &memento_p)
{
	CommandQueue<variant_t> *queue_p = get_queue(memento_p.e.mut(ecs), CommandQueue<variant_t>());
	if(queue_p)
	{
		queue_p->_current = memento_p._current;
		queue_p->_queued = memento_p._queued;
		queue_p->_queuedActions = memento_p._queuedActions;
		queue_p->_done = memento_p._done;
	}
}

}

#include "CommandQueueSystems.hh"