#pragma once

struct TimeStats
{
	double position_system = 0.;
	double tree_update = 0.;
	double attack_command = 0.;
	double attack_command_new_target = 0.;
};

#define START_TIME(name) const auto start_##name{std::chrono::high_resolution_clock::now()};
#define END_TIME(name) \
				const auto end_##name{std::chrono::high_resolution_clock::now()}; \
				const std::chrono::duration<double> elapsed_seconds_##name{end_##name - start_##name}; \
				time_stats_p.name += elapsed_seconds_##name.count() * 1000.;
