#pragma once

#include <list>
#include <variant>

#include "flecs.h"

#include "step/CommandQueueStep.hh"

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

template<typename variant_t>
struct CommandQueue
{
	typedef std::variant<
		CommandQueueDoneStep<variant_t>,
		CommandQueueAddStateStep<variant_t>,
		CommandQueueRemoveStateStep<variant_t>,
		CommandQueueSetCurrentStep<variant_t>,
		CommandQueueUpdateQueueStep<variant_t>,
		CommandQueuePopFrontQueueStep<variant_t>,
		CommandQueueUpdateComponentStep<variant_t>
	> CommandQueueStep;

	/// @warning this is just an out of date pointer
	/// to get the type information of the current action
	variant_t _current;
	std::list<variant_t> _queued;
	/// @brief true if the current command is done
	bool _done = false;

	/// @brief internal container for the old current
	/// value used to remove clean once current has been
	/// done
	variant_t _old = NoOpCommand();

	// PostLoad (1)
	// mark current has done if new command clean
	std::list<CommandQueueStep> set_current_done(NewCommand<variant_t> const &new_p)
	{
		std::list<CommandQueueStep> steps;
		if(new_p.clear)
		{
			steps.push_back(CommandQueueDoneStep<variant_t> {_done, true});
		}
		return steps;
	}

	// PostLoad (2)
	// set clean up state
	std::list<CommandQueueStep> clean_up_current(flecs::world &ecs, flecs::entity &e)
	{
		_old = NoOpCommand();
		std::list<CommandQueueStep> steps;
		if(_done && !std::holds_alternative<NoOpCommand>(_current))
		{
			// reset state
			steps.push_back(CommandQueueRemoveStateStep<variant_t> {state(ecs), _current, e});
			// add clean up
			steps.push_back(CommandQueueAddStateStep<variant_t> {cleanup(ecs), _current, e});
			// reset variant
			steps.push_back(CommandQueueSetCurrentStep<variant_t> {_current, NoOpCommand()});
			_old = _current;
		}
		steps.push_back(CommandQueueDoneStep<variant_t> {_done, false});
		return steps;
	}

	// OnUpdate (1)
	// update the current
	std::list<CommandQueueStep> update_from_new_command(NewCommand<variant_t> const &new_p)
	{
		std::list<CommandQueueStep> steps;
		// update queue
		if(!new_p.commands.empty())
		{
			CommandQueueUpdateQueueStep<variant_t> queue_update;
			queue_update.old_queue = _queued;
			// clear queue if necessary
			if(!new_p.clear)
			{
				queue_update.new_queue = _queued;
			}
			// update queue
			for(variant_t const &var_l : new_p.commands)
			{
				if(new_p.front)
				{
					queue_update.new_queue.push_front(var_l);
				}
				else
				{
					queue_update.new_queue.push_back(var_l);
				}
			}
			steps.push_back(queue_update);
		}
		return steps;
	}

	template<typename type_t>
	CommandQueueUpdateComponentStep<variant_t> update_comp(flecs::entity &e, type_t const &new_comp)
	{
		CommandQueueUpdateComponentStep<variant_t> step;
		type_t const * comp_l = e.get<type_t>();
		if(comp_l)
		{
			step.old_comp = *comp_l;
		}
		else
		{
			step.old_comp = NoOpCommand();
		}
		step.new_comp = new_comp;
		step.ent = e;

		return step;
	}

	// OnUpdate (2)
	std::list<CommandQueueStep> update_current(flecs::world &ecs, flecs::entity &e)
	{
		std::list<CommandQueueStep> steps;
		/// @todo warning here (may not work)
		steps.push_back(CommandQueueRemoveStateStep<variant_t> {cleanup(ecs), _old, e});
		if(std::holds_alternative<NoOpCommand>(_current) && !_queued.empty())
		{
			steps.push_back(CommandQueueSetCurrentStep<variant_t> {_current, _queued.front()});
			steps.push_back(CommandQueuePopFrontQueueStep<variant_t> {_queued.front()});

			// set state
			steps.push_back(CommandQueueAddStateStep<variant_t> {state(ecs), _queued.front(), e});

			// set component
			std::visit([this, &e, &steps](auto&& arg) { steps.push_back(update_comp(e, arg)); }, _queued.front());
		}
		return steps;
	}

	static flecs::entity state(flecs::world &ecs) { return ecs.entity("state"); }
	static flecs::entity cleanup(flecs::world &ecs) { return ecs.entity("cleanup"); }
};

template<typename variant_t>
void apply_all_command_queue_steps(
	std::list<typename CommandQueue<variant_t>::CommandQueueStep> &list_p,
	CommandQueue<variant_t> &queue_p)
{
	for(auto && step : list_p)
	{
		std::visit([&queue_p](auto&& arg) { arg.apply(queue_p); }, step);
	}
}

template<typename variant_t>
void set_up_command_queue_systems(flecs::world &ecs)
{
	// set up relations
    CommandQueue<variant_t>::state(ecs).add(flecs::Exclusive);
    CommandQueue<variant_t>::cleanup(ecs).add(flecs::Exclusive);

	ecs.system<NewCommand<variant_t> const, CommandQueue<variant_t>>()
		.kind(flecs::PostLoad)
		.each([](NewCommand<variant_t> const &new_p, CommandQueue<variant_t> &queue_p) {
			std::list<typename CommandQueue<variant_t>::CommandQueueStep> list = queue_p.set_current_done(new_p);
			apply_all_command_queue_steps(list, queue_p);
		});

	ecs.system<CommandQueue<variant_t>>()
		.kind(flecs::PostLoad)
		.write(CommandQueue<variant_t>::state(ecs), flecs::Wildcard)
		.write(CommandQueue<variant_t>::cleanup(ecs), flecs::Wildcard)
		.each([&ecs](flecs::entity e, CommandQueue<variant_t> &queue_p) {
			std::list<typename CommandQueue<variant_t>::CommandQueueStep> list = queue_p.clean_up_current(ecs, e);
			apply_all_command_queue_steps(list, queue_p);
		});

	ecs.system<NewCommand<variant_t> const, CommandQueue<variant_t>>()
		.kind(flecs::OnUpdate)
		.each([](flecs::entity e, NewCommand<variant_t> const &new_p, CommandQueue<variant_t> &queue_p) {
			std::list<typename CommandQueue<variant_t>::CommandQueueStep> list = queue_p.update_from_new_command(new_p);
			e.remove<NewCommand<variant_t>>();
			apply_all_command_queue_steps(list, queue_p);
		});

	ecs.system<CommandQueue<variant_t>>()
		.kind(flecs::OnUpdate)
		.write(CommandQueue<variant_t>::state(ecs), flecs::Wildcard)
		.write(CommandQueue<variant_t>::cleanup(ecs), flecs::Wildcard)
		.each([&ecs](flecs::entity e, CommandQueue<variant_t> &queue_p) {
			std::list<typename CommandQueue<variant_t>::CommandQueueStep> list = queue_p.update_current(ecs, e);
			apply_all_command_queue_steps(list, queue_p);
		});
}

}

#include "step/CommandQueueStep.defs.hh"
