#include "CommandQueue.hh"

namespace octopus
{



// PostLoad (1)
// mark current has done if new command clean
void CommandQueue::set_current_done(NewCommand const &new_p)
{
	if(new_p.clear)
	{
		_done = true;
	}
}

// PostLoad (2)
// set clean up state
void CommandQueue::clean_up_current(flecs::world &ecs, flecs::entity &e)
{
	if(_done && _current)
	{
		// reset state
		e.remove(state(ecs), ecs.entity(_current->naming()));
		// add clean up
		e.add(cleanup(ecs), ecs.entity(_current->naming()));
		// reset pointer
		_current.reset();
	}
	_done = false;
}

// OnUpdate (1)
// update the current
void CommandQueue::update_from_new_command(NewCommand const &new_p)
{
	// update queue
	if(!new_p.commands.empty())
	{
		// clear queue if necessary
		if(new_p.clear)
		{
			_queued.clear();
		}
		// update queue
		for(std::shared_ptr<Command> const &ptr_l : new_p.commands)
		{
			if(new_p.front)
			{
				_queued.push_front(ptr_l);
			}
			else
			{
				_queued.push_back(ptr_l);
			}
		}
	}
}

// OnUpdate (2)
void CommandQueue::update_current(flecs::world &ecs, flecs::entity &e)
{
	e.remove(cleanup(ecs), flecs::Wildcard);
	if(!_current && !_queued.empty())
	{
		std::shared_ptr<Command> next_l = _queued.front();
		_queued.pop_front();
		_current = next_l;
		e.add(state(ecs), ecs.entity(_current->naming()));
		_current->set(e);
	}
}

void set_up_command_queue_systems(flecs::world &ecs)
{
	// set up relations
    CommandQueue::state(ecs).add(flecs::Exclusive);
    CommandQueue::cleanup(ecs).add(flecs::Exclusive);

	ecs.system<NewCommand const, CommandQueue>()
		.kind(flecs::PostLoad)
		.each([](NewCommand const &new_p, CommandQueue &queue_p) {
			queue_p.set_current_done(new_p);
		});

	ecs.system<CommandQueue>()
		.kind(flecs::PostLoad)
		.write(CommandQueue::state(ecs), flecs::Wildcard)
		.write(CommandQueue::cleanup(ecs), flecs::Wildcard)
		.each([](flecs::entity e, CommandQueue &queue_p) {
			queue_p.clean_up_current(e.world(), e);
		});

	ecs.system<NewCommand const, CommandQueue>()
		.kind(flecs::OnUpdate)
		.each([](flecs::entity e, NewCommand const &new_p, CommandQueue &queue_p) {
			queue_p.update_from_new_command(new_p);
			e.remove<NewCommand>();
		});

	ecs.system<CommandQueue>()
		.kind(flecs::OnUpdate)
		.write(CommandQueue::state(ecs), flecs::Wildcard)
		.write(CommandQueue::cleanup(ecs), flecs::Wildcard)
		.each([](flecs::entity e, CommandQueue &queue_p) {
			queue_p.update_current(e.world(), e);
		});
}

}