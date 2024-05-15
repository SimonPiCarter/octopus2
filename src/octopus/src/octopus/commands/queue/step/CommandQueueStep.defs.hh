#include "CommandQueueStep.hh"
#include "octopus/commands/queue/CommandQueue.hh"

#include <iostream>

namespace octopus
{

template<typename variant_t>
void CommandQueueDoneStep<variant_t>::apply(CommandQueue<variant_t> &cQueue_p)
{
	cQueue_p._done = new_done;
}

template<typename variant_t>
void CommandQueueDoneStep<variant_t>::revert(CommandQueue<variant_t> &cQueue_p)
{
	cQueue_p._done = old_done;
}

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
void CommandQueueAddStateStep<variant_t>::apply(CommandQueue<variant_t> &)
{
	std::visit([this](auto&& arg) { add(ent, first, arg); }, second);
}

template<typename variant_t>
void CommandQueueAddStateStep<variant_t>::revert(CommandQueue<variant_t> &)
{
	std::visit([this](auto&& arg) { remove(ent, first, arg); }, second);
}

template<typename variant_t>
void CommandQueueRemoveStateStep<variant_t>::apply(CommandQueue<variant_t> &)
{
	std::visit([this](auto&& arg) { remove(ent, first, arg); }, second);
}

template<typename variant_t>
void CommandQueueRemoveStateStep<variant_t>::revert(CommandQueue<variant_t> &)
{
	std::visit([this](auto&& arg) { add(ent, first, arg); }, second);
}

template<typename variant_t>
void CommandQueueSetCurrentStep<variant_t>::apply(CommandQueue<variant_t> &cQueue_p)
{
	cQueue_p._current = new_current;
}

template<typename variant_t>
void CommandQueueSetCurrentStep<variant_t>::revert(CommandQueue<variant_t> &cQueue_p)
{
	cQueue_p._current = old_current;
}

template<typename variant_t>
void CommandQueueUpdateQueueStep<variant_t>::apply(CommandQueue<variant_t> &cQueue_p)
{
	cQueue_p._queued = new_queue;
}

template<typename variant_t>
void CommandQueueUpdateQueueStep<variant_t>::revert(CommandQueue<variant_t> &cQueue_p)
{
	cQueue_p._queued = old_queue;
}

template<typename variant_t>
void CommandQueuePopFrontQueueStep<variant_t>::apply(CommandQueue<variant_t> &cQueue_p)
{
	cQueue_p._queued.pop_front();
}

template<typename variant_t>
void CommandQueuePopFrontQueueStep<variant_t>::revert(CommandQueue<variant_t> &cQueue_p)
{
	cQueue_p._queued.push_front(front);
}

template<typename variant_t>
void CommandQueueUpdateComponentStep<variant_t>::apply(CommandQueue<variant_t> &cQueue_p)
{
	std::visit([this](auto&& arg) { ent.set(arg); }, new_comp);
}

template<typename variant_t>
void CommandQueueUpdateComponentStep<variant_t>::revert(CommandQueue<variant_t> &cQueue_p)
{
	std::visit([this](auto&& arg) { ent.set(arg); }, old_comp);
}

}
