#pragma once

#include <iostream>
#include <list>
#include <memory>

#include "flecs.h"

struct Com {
	virtual char const * const naming() const = 0;
	virtual void set(flecs::entity e) const = 0;
	virtual Com* clone() const = 0;
};

// System enablings commands should be part of the OnUpdate phase
// System cleaning up commands should be part of the PreUpdate phase

struct NewCommand
{
	/// @brief the commands to be added
	std::list<std::shared_ptr<Com>> commands;
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
// 	PostUpdate
// 	PreStore
// 	OnStore

struct CommandQueue
{
	/// @warning this is just an out of date pointer
	/// to get the type information of the current action
	std::shared_ptr<Com> _current;
	std::list<std::shared_ptr<Com>> _queued;
	/// @brief true if the current command is done
	bool _done = false;

	// PostLoad (1)
	// mark current has done if new command clean
	void set_current_done(NewCommand const &new_p)
	{
		if(new_p.clear)
		{
			_done = true;
		}
	}

	// PostLoad (2)
	// set clean up state
	void clean_up_current(flecs::world &ecs, flecs::entity &e)
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
	void update_from_new_command(NewCommand const &new_p)
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
			for(std::shared_ptr<Com> const &ptr_l : new_p.commands)
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
	void update_current(flecs::world &ecs, flecs::entity &e)
	{
		e.remove(cleanup(ecs), flecs::Wildcard);
		if(!_current && !_queued.empty())
		{
			std::shared_ptr<Com> next_l = _queued.front();
			_queued.pop_front();
			_current = next_l;
			e.add(state(ecs), ecs.entity(_current->naming()));
			_current->set(e);
		}
	}

	static flecs::entity state(flecs::world &ecs) { return ecs.entity("state"); }
	static flecs::entity cleanup(flecs::world &ecs) { return ecs.entity("cleanup"); }
};

void set_up_command_queue_systems(flecs::world &ecs);
