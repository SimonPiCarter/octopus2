#pragma once

#include "flecs.h"

namespace octopus
{

template<typename variant_t>
struct CommandQueue;

template<typename variant_t>
struct CommandQueueDoneStep
{
	bool old_done = false;
	bool new_done = false;

	void apply(CommandQueue<variant_t> &cQueue_p);
	void revert(CommandQueue<variant_t> &cQueue_p);
};

template<typename variant_t>
struct CommandQueueAddStateStep
{
	flecs::entity first;
	variant_t second;
	flecs::entity ent;

	void apply(CommandQueue<variant_t> &cQueue_p);
	void revert(CommandQueue<variant_t> &cQueue_p);
};

template<typename variant_t>
struct CommandQueueRemoveStateStep
{
	flecs::entity first;
	variant_t second;
	flecs::entity ent;

	void apply(CommandQueue<variant_t> &cQueue_p);
	void revert(CommandQueue<variant_t> &cQueue_p);
};

template<typename variant_t>
struct CommandQueueSetCurrentStep
{
	variant_t old_current;
	variant_t new_current;

	void apply(CommandQueue<variant_t> &cQueue_p);
	void revert(CommandQueue<variant_t> &cQueue_p);
};

template<typename variant_t>
struct CommandQueueUpdateQueueStep
{
	std::list<variant_t> old_queue;
	std::list<variant_t> new_queue;

	void apply(CommandQueue<variant_t> &cQueue_p);
	void revert(CommandQueue<variant_t> &cQueue_p);
};

template<typename variant_t>
struct CommandQueuePopFrontQueueStep
{
	variant_t front;

	void apply(CommandQueue<variant_t> &cQueue_p);
	void revert(CommandQueue<variant_t> &cQueue_p);
};

template<typename variant_t>
struct CommandQueueUpdateComponentStep
{
	variant_t old_comp;
	variant_t new_comp;
	flecs::entity ent;

	void apply(CommandQueue<variant_t> &cQueue_p);
	void revert(CommandQueue<variant_t> &cQueue_p);
};

} // namespace octopus
