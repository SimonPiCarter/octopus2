#include "StepContainer.hh"

namespace octopus
{

void reserve(StepContainer &container, size_t size)
{
	container.positions.steps.reserve(size);
	container.hitpoints.steps.reserve(size);
	container.attacks.steps.reserve(size);
	container.targets.steps.reserve(size);
}

void clear_container(StepContainer &container)
{
	container.positions.steps.clear();
	container.hitpoints.steps.clear();
	container.attacks.steps.clear();
	container.targets.steps.clear();
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
					apply_all(container[i].positions);
				});

				jobs_l.push_back([i, &container]() {
					apply_all(container[i].hitpoints);
				});

				jobs_l.push_back([i, &container]() {
					apply_all(container[i].attacks);
				});

				jobs_l.push_back([i, &container]() {
					apply_all(container[i].targets);
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
					revert_all(container[i].targets);
				});

				jobs_l.push_back([i, &container]() {
					revert_all(container[i].attacks);
				});

				jobs_l.push_back([i, &container]() {
					revert_all(container[i].hitpoints);
				});

				jobs_l.push_back([i, &container]() {
					revert_all(container[i].positions);
				});

				enqueue_and_wait(pool, jobs_l);
			}
		});
}

} // octopus
