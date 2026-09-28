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
	results_by_request.clear();
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

	Vector orig_centroid;
	Vector dest_centroid;
	if(!navigator->find_triangle_centroid(pos.pos, orig_centroid) ||
	   !navigator->find_triangle_centroid(target, dest_centroid))
	{
		END_TIME_PTR(query_path, stats)
		return {this, nullptr, pos.pos, target};
	}

	RequestKey const key {orig_centroid, dest_centroid};
	auto const found = results_by_request.find(key);
	if(found != results_by_request.end())
	{
		END_TIME_PTR(query_path, stats)
		return {this, found->second, pos.pos, target};
	}

	results.emplace_back();
	DelaunayPathResult *result = &results.back();
	results_by_request.emplace(key, result);
	requests.push_back({result, orig_centroid, dest_centroid});
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

		std::vector<std::size_t> path;
		if(navigator)
		{
			path = navigator->compute_path(request.orig_centroid,
			                               request.dest_centroid);
		}

		{
			std::lock_guard<std::mutex> lock(mutex);
			request.result->has_path = !path.empty();
			request.result->path = std::move(path);
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

	if(!cache->navigator)
	{
		END_TIME_PTR(path_funnelling, cache->stats)
		return {};
	}

	std::vector<Vector> const funnel =
		cache->navigator->compute_funnel_from_path(orig, dest, result->path);
	if(funnel.empty())
	{
		END_TIME_PTR(path_funnelling, cache->stats)
		return {};
	}
	Vector const direction =
		funnel.size() <= 2 ? dest - orig : funnel[1] - orig;
	END_TIME_PTR(path_funnelling, cache->stats)
	return direction;
}

}
