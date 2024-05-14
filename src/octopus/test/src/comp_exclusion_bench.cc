#include <gtest/gtest.h>

#include "flecs.h"

struct Walk { uint32_t t = 0; };
struct Runn { uint32_t t = 0; };
struct Attack { uint32_t t = 0; };

TEST(DISABLED_comp_exclusive, bench)
{
	flecs::world ecs;

	size_t nb = 1000000;

	for(size_t i = 0 ; i < nb ; ++ i)
	{
		auto e1 = ecs.entity();
		if(i % 3 == 0)
		{
			e1.set<Walk>({i%7});
		}
		if(i % 3 == 1)
		{
			e1.set<Runn>({i%12});
		}
		if(i % 3 == 2)
		{
			e1.set<Attack>({i%16});
		}
	}

	// System

	// Walk : walk for 7 progress then run
	ecs.system<Walk>()
		.iter([](flecs::iter& it, Walk *walk_p) {
			for (size_t i = 0; i < it.count(); i ++) {
				++walk_p[i].t;
				if(walk_p[i].t >= 7)
				{
					it.entity(i).set<Runn>({0});
					it.entity(i).remove<Walk>();
				}
			}
		});

	// Run : run for 12 progress then attack
	ecs.system<Runn>()
		.iter([](flecs::iter& it, Runn *run_p) {
			for (size_t i = 0; i < it.count(); i ++) {
				++run_p[i].t;
				if(run_p[i].t >= 12)
				{
					it.entity(i).set<Attack>({0});
					it.entity(i).remove<Runn>();
				}
			}
		});

	// Attack : attack for 16 progress then walk
	ecs.system<Attack>()
		.iter([](flecs::iter& it, Attack *attack_p) {
			for (size_t i = 0; i < it.count(); i ++) {
				++attack_p[i].t;
				if(attack_p[i].t >= 16)
				{
					it.entity(i).set<Walk>({0});
					it.entity(i).remove<Attack>();
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
