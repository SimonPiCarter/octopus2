#include "CommandQueueStep.hh"
#include "octopus/commands/queue/CommandQueue.hh"

#include <iostream>

namespace octopus
{

template<typename variant_t>
void CommandQueueDoneStep<variant_t>::apply(CommandQueue<variant_t> &cQueue_p)
{
	std::cout<<"CommandQueueDoneStep apply"<<" old "<<old_done<<" new "<<new_done<<std::endl;
	cQueue_p._done = new_done;
}

template<typename variant_t>
void CommandQueueDoneStep<variant_t>::revert(CommandQueue<variant_t> &cQueue_p)
{
	std::cout<<"CommandQueueDoneStep revert"<<" old "<<old_done<<" new "<<new_done<<std::endl;
	cQueue_p._done = old_done;
}

template<typename variant_t>
void CommandQueueAddStateStep<variant_t>::apply(CommandQueue<variant_t> &)
{
	std::cout<<"CommandQueueAddStateStep apply"<<std::endl;
	std::visit([this](auto&& arg) { add(ent, first, arg); }, second);
}

template<typename variant_t>
void CommandQueueAddStateStep<variant_t>::revert(CommandQueue<variant_t> &)
{
	std::cout<<"CommandQueueAddStateStep revert"<<std::endl;
	std::visit([this](auto&& arg) { remove(ent, first, arg); }, second);
}

template<typename variant_t>
void CommandQueueRemoveStateStep<variant_t>::apply(CommandQueue<variant_t> &)
{
	std::cout<<"CommandQueueRemoveStateStep apply"<<std::endl;
	std::visit([this](auto&& arg) { remove(ent, first, arg); }, second);
}

template<typename variant_t>
void CommandQueueRemoveStateStep<variant_t>::revert(CommandQueue<variant_t> &)
{
	std::cout<<"CommandQueueRemoveStateStep revert"<<std::endl;
	std::visit([this](auto&& arg) { add(ent, first, arg); }, second);
}

template<typename variant_t>
void CommandQueueSetCurrentStep<variant_t>::apply(CommandQueue<variant_t> &cQueue_p)
{
	std::cout<<"CommandQueueSetCurrentStep apply ";
	std::visit([this](auto&& arg) {
		std::cout<<arg.naming()<<" ";
	}, old_current);
	std::visit([this](auto&& arg) {
		std::cout<<arg.naming()<<" ";
	}, new_current);
	std::cout<<" "<<std::endl;
	cQueue_p._current = new_current;
}

template<typename variant_t>
void CommandQueueSetCurrentStep<variant_t>::revert(CommandQueue<variant_t> &cQueue_p)
{
	std::cout<<"CommandQueueSetCurrentStep revert ";
	std::visit([this](auto&& arg) {
		std::cout<<arg.naming()<<" ";
	}, old_current);
	std::visit([this](auto&& arg) {
		std::cout<<arg.naming()<<" ";
	}, new_current);
	std::cout<<" "<<std::endl;
	cQueue_p._current = old_current;
}

template<typename variant_t>
void CommandQueueUpdateQueueStep<variant_t>::apply(CommandQueue<variant_t> &cQueue_p)
{
	std::cout<<"CommandQueueUpdateQueueStep apply"<<std::endl;
	cQueue_p._queued = new_queue;
}

template<typename variant_t>
void CommandQueueUpdateQueueStep<variant_t>::revert(CommandQueue<variant_t> &cQueue_p)
{
	std::cout<<"CommandQueueUpdateQueueStep revert"<<std::endl;
	cQueue_p._queued = old_queue;
}

template<typename variant_t>
void CommandQueuePopFrontQueueStep<variant_t>::apply(CommandQueue<variant_t> &cQueue_p)
{
	std::cout<<"CommandQueuePopFrontQueueStep apply"<<std::endl;
	cQueue_p._queued.pop_front();
}

template<typename variant_t>
void CommandQueuePopFrontQueueStep<variant_t>::revert(CommandQueue<variant_t> &cQueue_p)
{
	std::cout<<"CommandQueuePopFrontQueueStep revert"<<std::endl;
	cQueue_p._queued.push_front(front);
}

template<typename variant_t>
void CommandQueueUpdateComponentStep<variant_t>::apply(CommandQueue<variant_t> &cQueue_p)
{
	std::cout<<"CommandQueueUpdateComponentStep apply"<<std::endl;
	std::visit([this](auto&& arg) { ent.set(arg); }, new_comp);
}

template<typename variant_t>
void CommandQueueUpdateComponentStep<variant_t>::revert(CommandQueue<variant_t> &cQueue_p)
{
	std::cout<<"CommandQueueUpdateComponentStep revert"<<std::endl;
	std::visit([this](auto&& arg) { ent.set(arg); }, old_comp);
}

}
