#pragma once

#include <gtest/gtest.h>

#include "octopus/commands/queue/CommandQueue.hh"
#include "octopus/components/step/StepReversal.hh"
#include "env/stream_ent.hh"

#include <vector>
#include <list>
#include <string>

struct StreamedEntityRecord
{
	std::list<std::string> records;

	std::string const &operator[](size_t idx_p) const
	{
		return *std::next(records.begin(), idx_p);
	}
};

/// @brief class used to check different components
/// can register all components for all registered entities
/// on each progress
/// Then revert everythin and compare outputs for each entity
/// @tparam ...Ts
template<class... Ts>
struct RevertTester
{
	RevertTester(std::vector<flecs::entity> const &tracked_p) : tracked_entities(tracked_p), records(tracked_p.size(), StreamedEntityRecord()) {}

	void add_record(flecs::world &ecs)
	{
		for(size_t i = 0 ; i < tracked_entities.size() ; ++ i)
		{
			flecs::entity const &e = tracked_entities[i];
			std::stringstream ss_l;
			stream_ent<Ts...>(ss_l, ecs, e);
			records[i].records.push_back(ss_l.str());

		}
		++recorded_steps;
	}

	template<class StepManager_t, class CommandMementoManager_t, class StateStepContainer_t>
	void revert_and_check_records(flecs::world &ecs, ThreadPool &pool_p,
		StepManager_t &step_manager_p, CommandMementoManager_t &command_memento_p,
		StateStepContainer_t & state_step_container_p)
	{
		// reverted records
		std::vector<StreamedEntityRecord> reverted_records(records.size(), StreamedEntityRecord());
		for(size_t i = 0; i < recorded_steps ; ++ i)
		{
			for(size_t i = 0 ; i < tracked_entities.size() ; ++ i)
			{
				flecs::entity const &e = tracked_entities[i];
				std::stringstream ss_l;
				stream_ent<Ts...>(ss_l, ecs, e);
				reverted_records[i].records.push_front(ss_l.str());
			}
			revert_n_steps(ecs, pool_p, 1, step_manager_p, command_memento_p, state_step_container_p);
			clear_n_steps(1, step_manager_p, command_memento_p, state_step_container_p);
		}

		for(size_t i = 0 ; i < tracked_entities.size() ; ++ i)
		{
			ASSERT_EQ(recorded_steps, reverted_records[i].records.size());
			ASSERT_EQ(recorded_steps, records[i].records.size());
			for(size_t step = 0 ; step < recorded_steps; ++step)
			{
				EXPECT_EQ(reverted_records[i][step], records[i][step]);
			}
		}
	}

private:
	std::vector<flecs::entity> tracked_entities;
	std::vector<StreamedEntityRecord> records;
	size_t recorded_steps = 0;
};
