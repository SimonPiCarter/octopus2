#include "StepContainer.hh"

namespace octopus
{

void reserve(StepContainer &container, size_t size)
{
	container.hitpoints.steps.reserve(size);
}

void clear_container(StepContainer &container)
{
	container.hitpoints.steps.clear();
}

void dispatch_apply(std::vector<StepContainer> &container, ThreadPool &pool)
{
	for(size_t i = 0 ; i < container.size(); ++ i)
	{
		std::vector<std::function<void()>> jobs_l;

		jobs_l.push_back([i, &container]() {
			apply_all(container[i].hitpoints);
		});

		enqueue_and_wait(pool, jobs_l);
	}
}

void dispatch_revert(std::vector<StepContainer> &container, ThreadPool &pool)
{
	for(size_t i = 0 ; i < container.size(); ++ i)
	{
		std::vector<std::function<void()>> jobs_l;

		jobs_l.push_back([i, &container]() {
			revert_all(container[i].hitpoints);
		});

		enqueue_and_wait(pool, jobs_l);
	}
}

} // octopus
