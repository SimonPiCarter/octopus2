// #include "flecs.h"
// #include <iostream>
// #include <sstream>
// #include <chrono>
// #include "utils/FixedPoint.hh"
// #include "utils/Grid.hh"

// #include "components/Movable.hh"
// #include "components/Attacker.hh"

// using namespace octopus;

// int main(int, char *[]) {
//     flecs::world ecs;
// 	ecs.set_threads(4);

//     ecs.component<Fixed>()
//         .member<long long>("_data");

//     // Register component with reflection data
//     ecs.component<Position>()
//         .member<Fixed>("x")
//         .member<Fixed>("y");

//     // Register component with reflection data
//     ecs.component<Velocity>()
//         .member<Fixed>("x")
//         .member<Fixed>("y");

// 	size_t nb_l = 15000;
// 	for(size_t i = 0 ; i < nb_l ; ++ i)
// 	{
// 		std::stringstream ss_l;
// 		ss_l<<"e"<<i;
// 		// Create a few test entities for a Position, Velocity query
// 		flecs::entity e = ecs.entity(ss_l.str().c_str());
// 			e.set<Position>({10, 20})
// 			.set<Velocity>({1, 2})
// 			.set<Attacker>({0,0,0,10,flecs::entity::null()})
// 			.set<Damageable>({100, 1});
// 	}

// 	// std::cout<<ecs.to_json()<<std::endl;

// 	Fixed target = 50;

// 	octopus::Grid grid_l;
// 	init(grid_l, 512, 512);

// 	flecs::system sys_init_grid = ecs.system<Position const>("init_grid")
// 	.multi_threaded()
//     .each([&grid_l](flecs::entity e, Position const& p) {
// 		set(grid_l, p.x.to_int(), p.y.to_int(), true);
//     });

// 	flecs::system sys_reset_grid = ecs.system<Position const>("reset_grid")
// 	.multi_threaded()
//     .each([&grid_l](flecs::entity e, Position const& p) {
// 		set(grid_l, p.x.to_int(), p.y.to_int(), false);
//     });

// 	flecs::system sys_attack = ecs.system<Position const, Velocity, Attacker>("Attack")
// 	.multi_threaded()
//     .each([target](flecs::entity e, Position const& p, Velocity &v, Attacker &a) {
// 		if(a.target != flecs::entity::null())
// 		{
// 			Position const *other = a.target.try_get<Position>();
// 			v.x = other->x - p.x;
// 			v.y = other->y - p.y;
// 		}
// 		else
// 		{
// 			v.x = target - p.x;
// 			v.y = target - p.y;
// 			Fixed sqr = numeric::square_root(v.x*v.x+v.y*v.y);
// 			v.x /= sqr;
// 			v.y /= sqr;
// 		}
//     });

// 	flecs::system sys_check_move = ecs.system<Position const, Velocity>("Check_Move")
// 	.multi_threaded()
//     .each([&grid_l](flecs::entity e, Position const & p, Velocity &v) {
//         Position new_pos {p.x + v.x, p.y + v.y };
// 		if(!is_free(grid_l, new_pos.x.to_int(), new_pos.y.to_int()))
// 		{
// 			// try orthogonal move
// 			std::swap(v.x, v.y);
// 			v.x = -v.x;
// 			Position new_new_pos {p.x + v.x, p.y + v.y };
// 			if(!is_free(grid_l, new_pos.x.to_int(), new_pos.y.to_int()))
// 			{
// 				v.x = 0;
// 				v.y = 0;
// 			}
// 		}
//     });

// 	flecs::system sys_move = ecs.system<Position, Velocity>("Move")
// 	.multi_threaded()
//     .each([](flecs::entity e, Position& p, Velocity &v) {
//         p.x += v.x;
//         p.y += v.y;
//     });

// 	sys_init_grid.run();

// 	auto start{std::chrono::steady_clock::now()};

// 	// loop
// 	{
// 		// attack routine
// 		sys_attack.run();

// 		// check grid
// 		sys_check_move.run();

// 		// reset grid
// 		sys_reset_grid.run();
// 		// move
// 		sys_move.run();
// 		// update grid
// 		sys_init_grid.run();
// 	}

//     auto end{std::chrono::steady_clock::now()};
//     std::chrono::duration<double> elapsed_seconds{end - start};

// 	std::cout<<"time "<<elapsed_seconds.count()*1000.<<"ms"<<std::endl;

// }

int main(int, char *[]) {
	return 0;
}