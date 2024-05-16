#pragma once

#include <list>
#include <variant>

#include "flecs.h"


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

template<typename variant_t>
struct NewCommand
{
	/// @brief the commands to be added
	std::list<variant_t> commands;
	/// @brief true if we place the command in front of the queue
	bool front = false;
	/// @brief true if we clear completely the queue
	bool clear = false;
};

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
void add(flecs::entity &e, flecs::entity &state, type_t const &)
{
	e.add_second<typename type_t::State>(state);
}

template<typename type_t>
void remove(flecs::entity &e, flecs::entity &state, type_t const &)
{
	e.remove_second<typename type_t::State>(state);
}

template<typename variant_t>
struct CommandQueue
{
	/// @warning this is just an out of date pointer
	/// to get the type information of the current action
	variant_t _current;
	std::list<variant_t> _queued;
	/// @brief true if the current command is done
	bool _done = false;

	// PostLoad (1)
	// mark current has done if new command clean
	void set_current_done(NewCommand<variant_t> const &new_p)
	{
		_done |= new_p.clear;
	}

	// PostLoad (2)
	// set clean up state
	void clean_up_current(flecs::world &ecs, flecs::entity &e)
	{
		if(_done && !std::holds_alternative<NoOpCommand>(_current))
		{
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

	// OnUpdate (1)
	// update the current
	void update_from_new_command(NewCommand<variant_t> const &new_p)
	{
		// update queue
		if(!new_p.commands.empty())
		{
			// clear queue if necessary
			if(new_p.clear)
			{
				_queued = new_p.commands;
			}
			else
			{
				// update queue
				for(variant_t const &var_l : new_p.commands)
				{
					if(new_p.front)
					{
						_queued.push_front(var_l);
					}
					else
					{
						_queued.push_back(var_l);
					}
				}
			}
		}
	}

	template<typename type_t>
	void update_comp(flecs::entity &e, type_t const &new_comp)
	{
		/// @todo use step here
		e.set(new_comp);
	}

	// OnUpdate (2)
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
void set_up_command_queue_systems(flecs::world &ecs)
{
	// set up relations
    CommandQueue<variant_t>::state(ecs).add(flecs::Exclusive);
    CommandQueue<variant_t>::cleanup(ecs).add(flecs::Exclusive);

	ecs.system<NewCommand<variant_t> const, CommandQueue<variant_t>>()
		.kind(flecs::PostLoad)
		.each([](NewCommand<variant_t> const &new_p, CommandQueue<variant_t> &queue_p) {
			queue_p.set_current_done(new_p);
		});

	ecs.system<CommandQueue<variant_t>>()
		.kind(flecs::PostLoad)
		.write(CommandQueue<variant_t>::state(ecs), flecs::Wildcard)
		.write(CommandQueue<variant_t>::cleanup(ecs), flecs::Wildcard)
		.each([&ecs](flecs::entity e, CommandQueue<variant_t> &queue_p) {
			queue_p.clean_up_current(ecs, e);
		});

	ecs.system<NewCommand<variant_t> const, CommandQueue<variant_t>>()
		.kind(flecs::OnUpdate)
		.each([](flecs::entity e, NewCommand<variant_t> const &new_p, CommandQueue<variant_t> &queue_p) {
			queue_p.update_from_new_command(new_p);
			e.remove<NewCommand<variant_t>>();
		});

	ecs.system<CommandQueue<variant_t>>()
		.kind(flecs::OnUpdate)
		.write(CommandQueue<variant_t>::state(ecs), flecs::Wildcard)
		.write(CommandQueue<variant_t>::cleanup(ecs), flecs::Wildcard)
		.each([&ecs](flecs::entity e, CommandQueue<variant_t> &queue_p) {
			queue_p.update_current(ecs, e);
		});
}

}
