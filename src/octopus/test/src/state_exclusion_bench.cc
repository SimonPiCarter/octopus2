#include <gtest/gtest.h>

#include "flecs.h"

struct Walk { uint32_t t = 0; };
struct Runn { uint32_t t = 0; };
struct Attack { uint32_t t = 0; };


TEST(DISABLED_state_exclusive, bench)
{
	flecs::world ecs;
    flecs::entity state = ecs.entity().add(flecs::Exclusive);
	// Basic state system
    flecs::entity walk = ecs.entity();
    flecs::entity run = ecs.entity();
	// extension
    flecs::entity attack = ecs.entity();

	size_t nb = 1000000;

	for(size_t i = 0 ; i < nb ; ++ i)
	{
		auto e1 = ecs.entity();
		if(i % 3 == 0)
		{
			e1.add(state, walk)
			.set<Walk>({i%7})
			.set<Runn>({0})
			.set<Attack>({0});
		}
		if(i % 3 == 1)
		{
			e1.add(state, run)
			.set<Walk>({0})
			.set<Runn>({i%12})
			.set<Attack>({0});
		}
		if(i % 3 == 2)
		{
			e1.add(state, attack)
			.set<Walk>({0})
			.set<Runn>({0})
			.set<Attack>({i%16});
		}
	}

	// System

	// Walk : walk for 7 progress then run
	ecs.system<Walk>()
		.with(state, walk)
		.iter([&state, &run](flecs::iter& it, Walk *walk_p) {
			for (size_t i = 0; i < it.count(); i ++) {
				++walk_p[i].t;
				if(walk_p[i].t >= 7)
				{
					walk_p[i].t = 0;
					it.entity(i).add(state, run);
				}
			}
		});

	// Run : run for 12 progress then attack
	ecs.system<Runn>()
		.with(state, run)
		.iter([&state, &attack](flecs::iter& it, Runn *run_p) {
			for (size_t i = 0; i < it.count(); i ++) {
				++run_p[i].t;
				if(run_p[i].t >= 12)
				{
					run_p[i].t = 0;
					it.entity(i).add(state, attack);
				}
			}
		});

	// Attack : attack for 16 progress then walk
	ecs.system<Attack>()
		.with(state, attack)
		.iter([&state, &walk](flecs::iter& it, Attack *attack_p) {
			for (size_t i = 0; i < it.count(); i ++) {
				++attack_p[i].t;
				if(attack_p[i].t >= 16)
				{
					attack_p[i].t = 0;
					it.entity(i).add(state, walk);
				}
			}
		});

	auto start = std::chrono::high_resolution_clock::now();

	for(size_t i = 0 ; i < 50 ; ++ i)
		ecs.progress();

	auto end = std::chrono::high_resolution_clock::now();
	auto diff = end - start;

	std::cout << "\tprogress\t" << std::chrono::duration_cast<std::chrono::milliseconds>(diff).count() << "ms" << std::endl;
}
