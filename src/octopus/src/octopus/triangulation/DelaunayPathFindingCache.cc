#include "DelaunayPathFindingCache.hh"

#include <utility>

namespace octopus
{

void DelaunayPathFindingCache::set_navigator(
	DelaunayTriangulationNavigator const &value)
{
	std::lock_guard<std::mutex> lock(mutex);
	navigator = &value;
	requests.clear();
	results.clear();
	results_by_destination.clear();
}

DelaunayPathQuery DelaunayPathFindingCache::query_path(
	Position const &pos, Vector const &target) const
{
	START_TIME(query_path)
	std::lock_guard<std::mutex> lock(mutex);
	if(!navigator)
	{
		END_TIME_PTR(query_path, stats)
		return {};
	}

	auto const found = results_by_destination.find(target);
	if(found != results_by_destination.end())
	{
		END_TIME_PTR(query_path, stats)
		return {this, found->second, pos.pos, target};
	}

	results.emplace_back();
	DelaunayPathResult *result = &results.back();
	results_by_destination.emplace(target, result);
	requests.push_back({result, pos.pos, target});
	END_TIME_PTR(query_path, stats)
	return {this, result, pos.pos, target};
}

void DelaunayPathFindingCache::compute_paths()
{
	START_TIME(path_finding)
	std::size_t run = 0;
	while(run < 10)
	{
		Request request;
		{
			std::lock_guard<std::mutex> lock(mutex);
			if(requests.empty())
			{
				break;
			}
			request = requests.front();
			requests.pop_front();
		}

		std::vector<Vector> funnel;
		if(navigator)
		{
			funnel = navigator->compute_funnel(request.orig, request.dest);
		}

		{
			std::lock_guard<std::mutex> lock(mutex);
			if(funnel.empty())
			{
				request.result->has_path = false;
			}
			else
			{
				request.result->has_path = true;
				request.result->direction =
					funnel.size() <= 2 ? request.dest - request.orig
					                   : funnel[1] - request.orig;
			}
			request.result->computed.store(true, std::memory_order_release);
		}
		++run;
	}
	END_TIME_PTR(path_finding, stats)
}

void DelaunayPathFindingCache::declare_cache_update_system(
	flecs::world &ecs, TimeStats &st)
{
	stats = &st;
	ecs.system<>()
		.kind(ecs.entity(PrepingUpdatePhase))
		.run([this](flecs::iter) { compute_paths(); });
}

bool DelaunayPathQuery::is_valid() const
{
	if(!cache)
	{
		return false;
	}
	return result &&
	       result->computed.load(std::memory_order_acquire) &&
	       result->has_path;
}

Vector DelaunayPathQuery::get_direction() const
{
	START_TIME(path_funnelling)
	if(!cache)
	{
		return {};
	}

	if(!result ||
	   !result->computed.load(std::memory_order_acquire) ||
	   !result->has_path)
	{
		END_TIME_PTR(path_funnelling, cache->stats)
		return {};
	}
	END_TIME_PTR(path_funnelling, cache->stats)
	return result->direction;
}

}
