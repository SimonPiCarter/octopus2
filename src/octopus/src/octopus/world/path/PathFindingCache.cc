#include "PathFindingCache.hh"

namespace octopus
{

/// @brief Compute a path request based on vector positions
PathRequest PathFindingCache::get_request(Triangulation const &triangulation, Vector const &orig, Vector const &dest) const
{
	PathRequest request {triangulation.cdt.triangles.size(), triangulation.cdt.triangles.size()};
	int idx_l = 0;

	for(CDT::Triangle const &tr : triangulation.cdt.triangles)
	{
		auto &&v = triangulation.cdt.vertices;
		auto &&trv = tr.vertices;

		bool found_orig = CDT::locatePointTriangle({orig.x,orig.y}, v[trv[0]], v[trv[1]], v[trv[2]]) != CDT::PtTriLocation::Outside;
		bool found_dest = CDT::locatePointTriangle({dest.x,dest.y}, v[trv[0]], v[trv[1]], v[trv[2]]) != CDT::PtTriLocation::Outside;
		if(found_orig)
		{
			request.orig = idx_l;
		}
		if(found_dest)
		{
			request.dest = idx_l;
		}

		if(request.orig < triangulation.cdt.triangles.size()
		&& request.dest < triangulation.cdt.triangles.size())
		{
			break;
		}

		++idx_l;
	}
	return request;
}

PathQuery PathFindingCache::query_path(flecs::world &ecs, Position const &pos, Vector const &target) const
{
	TriangulationPtr const *tr_ptr = ecs.get<TriangulationPtr>();
	// skip if no triangulation ptr
	if(!tr_ptr) { return PathQuery(); }
	Triangulation const *tr = tr_ptr->ptr;
	// skip if no triangulation
	if(!tr) { return PathQuery(); }

	// build request
	PathRequest request = get_request(*tr, pos.pos, target);
	// enqueue request
	list_requests.push_back(request);
	// return query
	return PathQuery { this, request.orig, request.dest, pos.pos, target };
}

std::vector<PathsInfo> const &PathFindingCache::get_paths_info() const { return paths_info; }

std::vector<std::size_t> PathFindingCache::build_path(std::size_t orig, std::size_t dest) const
{
	std::vector<std::size_t> path;

	std::size_t cur = orig;
	// while end or no more path
	while(cur != dest && cur < paths_info[dest].indexes.size())
	{
		// add current index
		path.push_back(cur);
		// update current index based on precomputed path
		cur = paths_info[dest].indexes[cur];
	}

	path.push_back(dest);
	return path;
}

void PathFindingCache::compute_paths(Triangulation const &tr)
{
	std::size_t const max_run = 10;
	std::size_t run = 0;
	while(!list_requests.empty() && run < max_run)
	{
		PathRequest const &request = list_requests.front();
		// check for calculation
		if(has_path(request.orig, request.dest))
		{
			// skip with no incrementation on the count of computation
			list_requests.pop_front();
			continue;
		}

		// compute path
		std::vector<std::size_t> path = tr.compute_path_from_idx(request.orig, request.dest);
		consolidate_path(path);

		// tidy up computations
		list_requests.pop_front();
		++run;
	}
}

bool PathFindingCache::has_path(std::size_t orig, std::size_t dest) const
{
	if(orig < paths_info.size() && dest < paths_info.size())
	{
		return paths_info[dest].indexes[orig] < paths_info.size();
	}
	return false;
}

void PathFindingCache::declare_cache_update_system(flecs::world &ecs)
{
	// update cache on each loop if necessary
	ecs.system<TriangulationPtr const>()
		.each([this](flecs::entity e, TriangulationPtr const &ptr) {
			if(paths_info.empty() || (ptr.ptr && ptr.ptr->revision != revision))
			{
				// default values
				const std::size_t nb_triangles = ptr.ptr->cdt.triangles.size();
				const PathsInfo empty_path_info {std::vector<std::size_t>(nb_triangles, nb_triangles)};
				// reset info to default values
				std::fill(paths_info.begin(), paths_info.end(), empty_path_info);
				paths_info.resize(nb_triangles, empty_path_info);
				list_requests.clear();
				// update revision
				revision = ptr.ptr->revision;
			}
		});

	// compute paths on each loop
	ecs.system<TriangulationPtr const>()
		.each([this](flecs::entity e, TriangulationPtr const &ptr) {
			if(!ptr.ptr) { return; }
			compute_paths(*ptr.ptr);
		});
}

/// @brief fill paths info from a path
void PathFindingCache::consolidate_path(std::vector<std::size_t> const &path)
{
	// return to avoid bad indexing
	if(path.empty())
	{
		return;
	}
	// get destination and init last to avoid treatment
	std::size_t dest = path[path.size()-1];
	std::size_t last = paths_info.size();
	for(std::size_t cur : path)
	{
		if(last < paths_info.size())
		{
			paths_info[dest].indexes[last] = cur;
		}
		last = cur;
	}
}

bool PathQuery::is_valid() const
{
	return cache && cache->has_path(orig, dest);
}

Vector PathQuery::get_direction(flecs::world &ecs) const
{
	std::vector<std::size_t> path = cache->build_path(orig, dest);

	TriangulationPtr const *tr_ptr = ecs.get<TriangulationPtr>();
	// skip if no triangulation ptr
	if(!tr_ptr) { return vert_dest - vert_orig; }
	Triangulation const *tr = tr_ptr->ptr;
	if(!tr) { return vert_dest - vert_orig; }

	std::vector<Vector> funnel = tr->compute_funnel_from_path(vert_orig, vert_dest, path);
	if(funnel.size() < 2)
	{
		return vert_dest - vert_orig;
	}
	return funnel[1] - funnel[0];
}

}
