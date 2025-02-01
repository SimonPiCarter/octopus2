#pragma once

#include <chrono>
#include <atomic>

struct TimeStats
{
	std::atomic_int64_t position_system = 0.;
	std::atomic_int64_t tree_update = 0.;
	std::atomic_int64_t move_command = 0.;
	std::atomic_int64_t attack_command = 0.;
	std::atomic_int64_t attack_command_new_target = 0.;
	uint64_t moving_entities = 0;
	std::atomic_int64_t path_finding = 0.;
	std::atomic_int64_t path_funnelling = 0.;
	std::atomic_int64_t query_path = 0.;
	std::vector<double> unamed_timers;

	void operator=(TimeStats const &ts)
	{
		position_system.store(ts.position_system.load());
		tree_update.store(ts.tree_update.load());
		move_command.store(ts.move_command.load());
		attack_command.store(ts.attack_command.load());
		attack_command_new_target.store(ts.attack_command_new_target.load());
		path_finding.store(ts.path_finding.load());
		path_funnelling.store(ts.path_funnelling.load());
		query_path.store(ts.query_path.load());
		moving_entities = ts.moving_entities;
		unamed_timers = ts.unamed_timers;
	}
};

struct TimeStatsPtr
{
	TimeStats * ptr = nullptr;
};

#define START_TIME(name) const auto start_##name{std::chrono::high_resolution_clock::now()};
#define END_TIME(name) \
				const auto end_##name{std::chrono::high_resolution_clock::now()}; \
				const std::chrono::nanoseconds elapsed_seconds_##name{end_##name - start_##name}; \
				time_stats_p.name += elapsed_seconds_##name.count();
#define END_TIME_PTR(name, ptr_name) \
				const auto end_##name{std::chrono::high_resolution_clock::now()}; \
				const std::chrono::nanoseconds elapsed_seconds_##name{end_##name - start_##name}; \
				if(ptr_name) ptr_name->name += elapsed_seconds_##name.count();
#define END_TIME_UNAMED(index) \
				const auto end_##index{std::chrono::high_resolution_clock::now()}; \
				const std::chrono::nanoseconds elapsed_seconds_##index{end_##index - start_##index}; \
				if(time_stats_p.unamed_timers.size() <= index) time_stats_p.unamed_timers.resize(index+1, 0.); \
				time_stats_p.unamed_timers.at(index) += elapsed_seconds_##index.count();
#define END_TIME_PTR_UNAMED(index, ptr_name) \
				const auto end_##index{std::chrono::high_resolution_clock::now()}; \
				const std::chrono::nanoseconds elapsed_seconds_##index{end_##index - start_##index}; \
				if(ptr_name && ptr_name->unamed_timers.size() <= index) ptr_name->unamed_timers.resize(index+1, 0.); \
				if(ptr_name) ptr_name->unamed_timers.at(index) += elapsed_seconds_##index.count();
