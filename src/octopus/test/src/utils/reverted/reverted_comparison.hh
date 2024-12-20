#pragma once

#include <gtest/gtest.h>

#include "octopus/commands/queue/CommandQueue.hh"
#include "octopus/components/step/StepReversal.hh"
#include "octopus/world/WorldContext.hh"
#include "env/stream_ent.hh"

#include <vector>
#include <list>
#include <string>
#include <sstream>

/// @brief class used to check different components
/// can register all components for all registered entities
/// on each progress
/// Then revert everythin and compare outputs for each entity
/// @tparam ...Ts
template<typename variant_t, class... Ts>
struct RevertTester
{
	RevertTester(std::vector<flecs::entity> const &tracked_p) : tracked_entities(tracked_p), records(tracked_p.size(), StreamedEntityRecord()) {}

	void add_second_recorder(flecs::entity first)
	{
		tracked_pairs.push_back(first);
	}

	void add_record(flecs::world &ecs)
	{
		for(size_t i = 0 ; i < tracked_entities.size() ; ++ i)
		{
			flecs::entity const &e = tracked_entities[i];
			if(!e.is_alive()) { continue; }
			std::stringstream ss_l;
			stream_ent<Ts...>(ss_l, ecs, e);
			for(flecs::entity const &first_l : tracked_pairs)
			{
				stream_second_components(ss_l, e, first_l, variant_t());
			}
			records[i].records.push_back(ss_l.str());

		}
		++recorded_steps;
	}

	template<class StepContext_t>
	void revert_and_check_records(octopus::WorldContext<typename StepContext_t::step> &world, StepContext_t &stepContext_p)
	{
		// reverted records
		std::vector<StreamedEntityRecord> reverted_records(records.size(), StreamedEntityRecord());
		for(size_t i = 0; i < recorded_steps ; ++ i)
		{
			for(size_t i = 0 ; i < tracked_entities.size() ; ++ i)
			{
				flecs::entity const &e = tracked_entities[i];
				if(!e.is_alive()) { continue; }
				std::stringstream ss_l;
				stream_ent<Ts...>(ss_l, world.ecs, e);
				for(flecs::entity const &first_l : tracked_pairs)
				{
					stream_second_components(ss_l, e, first_l, variant_t());
				}
				reverted_records[i].records.push_front(ss_l.str());
			}
			revert_n_steps(world.ecs, world.pool, 1, stepContext_p.step_manager, stepContext_p.memento_manager, stepContext_p.state_step_manager);
			clear_n_steps(world.ecs, 1, stepContext_p.step_manager, stepContext_p.memento_manager, stepContext_p.state_step_manager);
		}

		for(size_t i = 0 ; i < tracked_entities.size() ; ++ i)
		{
			ASSERT_EQ(recorded_steps, reverted_records[i].records.size());
			ASSERT_EQ(recorded_steps, records[i].records.size());
			for(size_t step = 0 ; step < recorded_steps; ++step)
			{
				// std::cout<<"p "<<records[i][step]<<std::endl;
				// std::cout<<"r "<<reverted_records[i][step]<<std::endl<<std::endl;
				EXPECT_EQ(reverted_records[i][step], records[i][step]) << "error while comparing step "<< step;
			}
		}
	}

	bool operator==(RevertTester const other) const
	{
		return records == other.records;
	}

	std::vector<StreamedEntityRecord> const &get_records() const { return records; }
private:
	std::vector<flecs::entity> tracked_entities;
	std::vector<flecs::entity> tracked_pairs;
	std::vector<StreamedEntityRecord> records;
	size_t recorded_steps = 0;
};

namespace std
{
	std::ostream &operator<<(std::ostream &oss, StreamedEntityRecord const &rec);

	template<typename variant_t, class... Ts>
	std::ostream &operator<<(std::ostream &oss, RevertTester<variant_t, Ts...> const &tester)
	{
		oss << "RevertTester[";
		std::for_each(tester.get_records().begin(), tester.get_records().end(), [&oss](StreamedEntityRecord const &entry)
		{
			oss << entry <<", ";
		});
		oss <<"]";
		return oss;
	}
} // namespace std
