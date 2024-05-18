#pragma once

#include <list>
#include <variant>

#include "flecs.h"

#include "action/CommandQueueAction.hh"

namespace octopus
{

/// @brief minimal implementation for a command that can be in the queue
struct NoOpCommand {
	int32_t no_op = 0;

	static constexpr char const * const naming()  { return "no_op"; }
	struct State {};
};

// System running commands should be part of the OnUpdate phase
// System cleaning up commands should be part of the PreUpdate phase

// phases order :
// 	OnLoad (loading input)
// 	PostLoad (setting up from intput & cleaning up) : marking current has done and tagging clean up and removing state
// 	PreUpdate - all clean ups from commands
// 	OnUpdate (loading new commands) : updating state based on queue
// 	OnValidate - all actions from commands
// 	PostUpdate - apply all steps
// 	PreStore
// 	OnStore

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

	// PostLoad (1)
	// set clean up state
	void clean_up_current(flecs::world &ecs, flecs::entity &e)
	{
		if(_done && !std::holds_alternative<NoOpCommand>(_current))
		{
			/// @todo use step here
			// reset state
			std::visit([this, &ecs, &e](auto&& arg) { remove(e, state(ecs), arg); }, _current);
			// add clean up
			std::visit([this, &ecs, &e](auto&& arg) { add(e, cleanup(ecs), arg); }, _current);
			// reset variant
			_current = NoOpCommand();
		}
		// reset done
		_done = false;
	}

	template<typename type_t>
	void update_comp(flecs::entity &e, type_t const &new_comp)
	{
		/// @todo use step here
		e.set(new_comp);
	}

	// OnUpdate (1)
	void update_current(flecs::world &ecs, flecs::entity &e)
	{
		e.remove(cleanup(ecs), flecs::Wildcard);
		if(std::holds_alternative<NoOpCommand>(_current) && !_queued.empty())
		{
			_current = _queued.front();
			_queued.pop_front();

			// set state
			std::visit([this, &ecs, &e](auto&& arg) { add(e, state(ecs), arg); }, _current);

			// set component
			std::visit([this, &e](auto&& arg) { update_comp(e, arg); }, _current);
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
	return e.get_mut<queue_t>();
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