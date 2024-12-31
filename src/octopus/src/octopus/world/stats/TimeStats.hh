#pragma once

#include <chrono>

struct TimeStats
{
	double position_system = 0.;
	double tree_update = 0.;
	double attack_command = 0.;
	double attack_command_new_target = 0.;
	uint64_t moving_entities = 0;
	double path_finding = 0.;
	double path_funnelling = 0.;
};

struct TimeStatsPtr
{
	TimeStats * ptr = nullptr;
};

#define START_TIME(name) const auto start_##name{std::chrono::high_resolution_clock::now()};
#define END_TIME(name) \
				const auto end_##name{std::chrono::high_resolution_clock::now()}; \
				const std::chrono::duration<double> elapsed_seconds_##name{end_##name - start_##name}; \
				time_stats_p.name += elapsed_seconds_##name.count() * 1000.;
#define END_TIME_ECS(name) \
				const auto end_##name{std::chrono::high_resolution_clock::now()}; \
				const std::chrono::duration<double> elapsed_seconds_##name{end_##name - start_##name}; \
				if(ecs.get<TimeStatsPtr>()) ecs.get<TimeStatsPtr>()->ptr->name += elapsed_seconds_##name.count() * 1000.;
