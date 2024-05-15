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

void declare_apply_system(flecs::world &ecs, std::vector<StepContainer> &container, ThreadPool &pool)
{
	ecs.system()
		.kind<Apply>()
		.iter([&pool, &container](flecs::iter& it) {
			for(size_t i = 0 ; i < container.size(); ++ i)
			{
				std::vector<std::function<void()>> jobs_l;

				jobs_l.push_back([i, &container]() {
					apply_all(container[i].hitpoints);
				});

				enqueue_and_wait(pool, jobs_l);
			}
		});
}

void declare_revert_system(flecs::world &ecs, std::vector<StepContainer> &container, ThreadPool &pool)
{
	ecs.system()
		.kind<Revert>()
		.iter([&pool, &container](flecs::iter& it) {
			for(size_t i = 0 ; i < container.size(); ++ i)
			{
				std::vector<std::function<void()>> jobs_l;

				jobs_l.push_back([i, &container]() {
					revert_all(container[i].hitpoints);
				});

				enqueue_and_wait(pool, jobs_l);
			}
		});
}

} // octopus
