#pragma once

#include <chrono>

struct TimeStats
{
	double position_system = 0.;
	double tree_update = 0.;
	double move_command = 0.;
	double attack_command = 0.;
	double attack_command_new_target = 0.;
	uint64_t moving_entities = 0;
	double path_finding = 0.;
	double path_funnelling = 0.;
	double query_path = 0.;
	std::vector<double> unamed_timers;
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
#define END_TIME_PTR(name, ptr_name) \
				const auto end_##name{std::chrono::high_resolution_clock::now()}; \
				const std::chrono::duration<double> elapsed_seconds_##name{end_##name - start_##name}; \
				if(ptr_name) ptr_name->name += elapsed_seconds_##name.count() * 1000.;
#define END_TIME_UNAMED(index) \
				const auto end_##index{std::chrono::high_resolution_clock::now()}; \
				const std::chrono::duration<double> elapsed_seconds_##index{end_##index - start_##index}; \
				if(time_stats_p.unamed_timers.size() <= index) time_stats_p.unamed_timers.resize(index+1, 0.); \
				time_stats_p.unamed_timers.at(index) += elapsed_seconds_##index.count() * 1000.;
#define END_TIME_PTR_UNAMED(index, ptr_name) \
				const auto end_##index{std::chrono::high_resolution_clock::now()}; \
				const std::chrono::duration<double> elapsed_seconds_##index{end_##index - start_##index}; \
				if(ptr_name && ptr_name->unamed_timers.size() <= index) ptr_name->unamed_timers.resize(index+1, 0.); \
				if(ptr_name) ptr_name->unamed_timers.at(index) += elapsed_seconds_##index.count() * 1000.;
