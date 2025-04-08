#include "Triangulation.hh"

#include <algorithm>
#include <chrono>

#include "cdt/include/ShortestPath.h"

namespace octopus {

void Triangulation::reset()
{
	// reset all
	cdt = CDT::Triangulation<octopus::Fixed>(CDT::detail::defaults::vertexInsertionOrder, CDT::IntersectingConstraintEdges::TryResolve, octopus::Fixed::One()/100);
	edges.clear();
	forbidden_triangles.clear();
	init(_size_x, _size_y);
}

void Triangulation::init(int x, int y)
{
	_size_x = x;
	_size_y = y;

	cdt.insertVertices({{-100,-100},{x+100,-100},{x+100,y+100},{-100,y+100},
	});
	insert_edge(3,0);
	insert_edge(2,3);
	insert_edge(1,2);
	insert_edge(0,1);
}

int Triangulation::insert_point(int x, int y, int forbidden)
{
	int i = 0;
	for(auto &&v : cdt.vertices)
	{
		if(v.x.to_int() ==x && v.y.to_int() ==y)
		{
			return i-3;
		}
		++i;
	}
	cdt.insertVertices({{x,y}});
	int idx = cdt.vertices.size()-4;

	forbidden_triangles.resize(cdt.triangles.size(), false);

	return idx;
}

void Triangulation::insert_edge(int idx_point_1, int idx_point_2)
{
	cdt.insertEdges({CDT::Edge(idx_point_1,idx_point_2)});
	edges.push_back(std::make_pair(idx_point_1+3, idx_point_2+3));
	forbidden_triangles.resize(cdt.triangles.size(), false);
}

bool containsPoint(CDT::V2d<octopus::Fixed> lineP, CDT::V2d<octopus::Fixed> lineQ, CDT::V2d<octopus::Fixed> point)
{
    bool xBetween = ((point.x - lineP.x) * (point.x - lineQ.x) <= 0.);
    bool yBetween = ((point.y - lineP.y) * (point.y - lineQ.y) <= 0.);
    if (!xBetween || !yBetween) { // early return or can be moved to the end
        return false;
    }
    octopus::Fixed dxPQ = lineQ.x - lineP.x;
    octopus::Fixed dyPQ = lineQ.y - lineP.y;
    octopus::Fixed lineLengthSquared = dxPQ*dxPQ + dyPQ*dyPQ;
    octopus::Fixed crossproduct = (point.y - lineP.y) * dxPQ - (point.x - lineP.x) * dyPQ;
    return crossproduct * crossproduct <= lineLengthSquared;
}

void Triangulation::finalize()
{
	forbidden_triangles.clear();
	forbidden_triangles.resize(cdt.triangles.size(), false);
	size_t i = 0;
	for(CDT::Triangle const &tr : cdt.triangles)
	{
		for(auto const &edge_l : edges)
		{
			bool p0_on_edge_l = containsPoint(cdt.vertices[edge_l.first], cdt.vertices[edge_l.second], cdt.vertices[tr.vertices[0]]);
			bool p1_on_edge_l = containsPoint(cdt.vertices[edge_l.first], cdt.vertices[edge_l.second], cdt.vertices[tr.vertices[1]]);
			bool p2_on_edge_l = containsPoint(cdt.vertices[edge_l.first], cdt.vertices[edge_l.second], cdt.vertices[tr.vertices[2]]);
			if((p0_on_edge_l && p1_on_edge_l)
			|| (p0_on_edge_l && p2_on_edge_l)
			|| (p1_on_edge_l && p2_on_edge_l))
			{
				CDT::VertInd v = tr.vertices[0];
				if(p0_on_edge_l && p1_on_edge_l)
				{
					v = tr.vertices[2];
				}
				if(p0_on_edge_l && p2_on_edge_l)
				{
					v = tr.vertices[1];
				}
				CDT::PtLineLocation::Enum pos = CDT::locatePointLine(cdt.vertices[v], cdt.vertices[edge_l.first], cdt.vertices[edge_l.second]);
				if(pos == CDT::PtLineLocation::Right)
				{
					forbidden_triangles[i] = true;
				}
			}
		}
		++i;
	}
	// cdt.eraseOuterTrianglesAndHoles();
}

std::vector<std::size_t> Triangulation::compute_path(Vector const &orig, Vector const &dest) const
{
	CDT::TriInd orig_idx = cdt.get_triangle({orig.x,orig.y})[0];
	CDT::TriInd dest_idx = cdt.get_triangle({dest.x,dest.y})[0];
	if(orig_idx != dest_idx)
	{
		orig_idx = cdt.get_closest_non_tagged_triangle(orig_idx, {orig.x,orig.y}, forbidden_triangles);
	}

	if(orig_idx == CDT::noNeighbor || dest_idx == CDT::noNeighbor)
	{
		return {};
	}

	return compute_path_from_idx(orig_idx, dest_idx);
}

std::vector<std::size_t> Triangulation::compute_path_from_idx(std::size_t orig, std::size_t dest) const
{
	return CDT::shortest_path(cdt, orig, dest, forbidden_triangles);
}

std::vector<Vector> Triangulation::compute_funnel(Vector const &orig, Vector const &dest) const
{
	std::vector<std::size_t> path = compute_path(orig, dest);
	return compute_funnel_from_path(orig, dest, path);
}

std::vector<Vector> Triangulation::compute_funnel_from_path(Vector const &orig, Vector const &dest, std::vector<std::size_t> const &path) const
{
	if(path.empty())
	{
		return {orig, dest};
	}

	std::vector<CDT::V2d<octopus::Fixed>> funnel = CDT::funnel_algorithm(
		cdt,
		path,
		CDT::V2d<octopus::Fixed>{orig.x,orig.y},
		CDT::V2d<octopus::Fixed>{dest.x,dest.y}
	);

	std::vector<Vector> result;
	for(auto vert : funnel)
	{
		result.push_back(Vector(vert.x, vert.y));
	}

	return result;
}

CDT::FunnelDebug<octopus::Fixed> Triangulation::debug_funnel(Vector const &orig, Vector const &dest, int step) const
{
	CDT::FunnelDebug<octopus::Fixed> debug {step};

	std::vector<std::size_t> path = compute_path(orig, dest);
	if(!path.empty())
	{
		std::vector<CDT::V2d<octopus::Fixed>> funnel = CDT::funnel_algorithm(
			cdt,
			path,
			CDT::V2d<octopus::Fixed>{orig.x,orig.y},
			CDT::V2d<octopus::Fixed>{dest.x,dest.y},
			&debug
		);
	}

	return debug;
}

void insert_box(Triangulation &tr, int x, int y, int size_x, int size_y, bool forbidden)
{
	int indexes[4] = {
		tr.insert_point(x       , y       , forbidden),
		tr.insert_point(x+size_x, y       , forbidden),
		tr.insert_point(x+size_x, y+size_y, forbidden),
		tr.insert_point(x       , y+size_y, forbidden),
	};

	tr.insert_edge(indexes[0],indexes[3]);
	tr.insert_edge(indexes[3],indexes[2]);
	tr.insert_edge(indexes[2],indexes[1]);
	tr.insert_edge(indexes[1],indexes[0]);
}

struct EdgeTriangleSurface
{
	std::array<CDT::VertInd, 2> edge;
	std::array<CDT::VertInd, 2> oriented_edge;
	CDT::TriInd triangle = 0;
	uint8_t neighbour_idx = 0;
};

struct Unmatched {
	size_t idx = 0;
	std::vector<CDT::VertInd> edge;
};

std::set<CDT::TriInd> get_neighbors(Triangulation &triangulation, CDT::TriInd orig_idx, CDT::VertInd vi)
{
	std::set<CDT::TriInd> neighbors;
	std::vector<CDT::TriInd> open_neighbors {orig_idx};
	while(!open_neighbors.empty())
	{
		CDT::TriInd idx = open_neighbors.back();
		neighbors.insert(idx);
		open_neighbors.pop_back();

		CDT::Triangle &cur_tr = triangulation.cdt.triangles[idx];

		for(auto n : cur_tr.neighbors)
		{
			if(n == CDT::noNeighbor)
			{
				continue;
			}
			bool contains_v = false;
			for(CDT::VertInd cur : triangulation.cdt.triangles[n].vertices)
			{
				if(cur == vi)
				{
					contains_v = true;
					break;
				}
			}
			if(contains_v
			&& std::find(neighbors.begin(), neighbors.end(), n) == neighbors.end()
			&& std::find(open_neighbors.begin(), open_neighbors.end(), n) == open_neighbors.end())
			{
				open_neighbors.push_back(n);
			}
		}
	}

	return neighbors;
}

std::vector<EdgeTriangleSurface> get_surface(Triangulation &triangulation, std::set<CDT::TriInd> const &neighbors, CDT::VertInd vi)
{
	std::vector<EdgeTriangleSurface> surface;

	for(CDT::TriInd const &n : neighbors)
	{
		std::vector<CDT::VertInd> edge;
		size_t index_vi = 0;
		for( ; index_vi < 3 ; ++ index_vi)
		{
			if(triangulation.cdt.triangles[n].vertices[index_vi] == vi)
			{
				break;
			}
		}
		edge.push_back(triangulation.cdt.triangles[n].vertices[(index_vi+1) % 3]);
		edge.push_back(triangulation.cdt.triangles[n].vertices[(index_vi+2) % 3]);
		CDT::TriInd contact = triangulation.cdt.triangles.size();
		for(CDT::TriInd const &cur : triangulation.cdt.triangles[n].neighbors)
		{
			if(std::find(neighbors.begin(), neighbors.end(), cur) == neighbors.end())
			{
				if(contact != triangulation.cdt.triangles.size())
				{
					assert(false);
					printf("error? two contact?\n");
				}
				contact = cur;
			}
		}
		std::vector<CDT::VertInd> sorted_edge = edge;
		std::sort(sorted_edge.begin(), sorted_edge.end());

		uint8_t neighbour_idx = 0;
		CDT::Triangle &tr_contact = triangulation.cdt.triangles[contact];
		if(tr_contact.neighbors[1] == n)
		{
			neighbour_idx = 1;
		}
		if(tr_contact.neighbors[2] == n)
		{
			neighbour_idx = 2;
		}

		EdgeTriangleSurface info {
			{sorted_edge[0], sorted_edge[1]},
			{edge[0], edge[1]},
			contact,
			neighbour_idx
		};
		surface.push_back(std::move(info));
	}

	return surface;
}

std::unordered_map<CDT::VertInd, CDT::VertInd> triangulate(octopus::Triangulation &ref_tr, octopus::Triangulation &tr, std::vector<EdgeTriangleSurface> &surface)
{
	std::unordered_map<CDT::VertInd, CDT::VertInd> added_points;
	for(EdgeTriangleSurface const &info : surface)
	{
		auto &&v1 = ref_tr.cdt.vertices[info.edge[0]];
		auto &&v2 = ref_tr.cdt.vertices[info.edge[1]];
		CDT::VertInd idx1 = tr.insert_point(v1.x.to_int(), v1.y.to_int(), 0);
		CDT::VertInd idx2 = tr.insert_point(v2.x.to_int(), v2.y.to_int(), 0);
		tr.insert_edge(idx1, idx2);
		added_points[idx1+3] = info.edge[0];
		added_points[idx2+3] = info.edge[1];
	}
	return added_points;
}

std::vector<CDT::TriInd> get_triangles(
	octopus::Triangulation &tr,
	std::vector<EdgeTriangleSurface> const &surface,
	std::unordered_map<CDT::VertInd, CDT::VertInd> &map
) {
	std::vector<CDT::TriInd> triangles;
	CDT::TriInd cur_idx = 0;
	for(CDT::Triangle const &triangle : tr.cdt.triangles)
	{
		bool matching_surface = false;
		auto &&v0 = triangle.vertices[0];
		auto &&v1 = triangle.vertices[1];
		auto &&v2 = triangle.vertices[2];
		if(map.find(v0) != map.end() && map.find(v1) != map.end() && map.find(v2) != map.end())
		{
			matching_surface = true;
		}
		if(map.find(v0) == map.end() || map.find(v1) == map.end() || map.find(v2) == map.end())
		{
			++cur_idx;
			continue;
		}

		// unmatch if we match a revert oriented edge from the surface
		for(EdgeTriangleSurface const &surface_part : surface)
		{
			if((map[v0] == surface_part.oriented_edge[1] && map[v1] == surface_part.oriented_edge[0])
			|| (map[v1] == surface_part.oriented_edge[1] && map[v2] == surface_part.oriented_edge[0])
			|| (map[v2] == surface_part.oriented_edge[1] && map[v0] == surface_part.oriented_edge[0]))
			{
				matching_surface = false;
			}
		}
		if(matching_surface)
		{
			triangles.push_back(cur_idx);
		}
		++cur_idx;
	}

	return triangles;
}

std::vector<CDT::VertInd> get_contact_edge(CDT::Triangle const &other_triangle, CDT::Triangle const &triangle, std::unordered_map<CDT::VertInd, CDT::VertInd> const &added_points_map)
{
	std::vector<CDT::VertInd> ind = {};

	for(auto &&other_v : other_triangle.vertices)
	{
		for(auto &&v : triangle.vertices)
		{
			auto it = added_points_map.find(other_v);
			if(it != added_points_map.end() && v == it->second)
			{
				ind.push_back(it->second);
				break;
			}
		}
	}

	std::sort(ind.begin(), ind.end());
	assert(ind.size() == 2);
	return ind;
}

void remove_point(Triangulation &triangulation, int x, int y)
{
	// get triangle
	CDT::TriInd orig_idx = triangulation.cdt.get_triangle({x, y})[0];
	CDT::Triangle &tr = triangulation.cdt.triangles[orig_idx];
	CDT::V2d<octopus::Fixed> v {x,y};
	CDT::VertInd vi = triangulation.cdt.vertices.size();
	for(CDT::VertInd cur : tr.vertices)
	{
		if(triangulation.cdt.vertices[cur] == v)
		{
			vi = cur;
		}
	}
	if(vi == triangulation.cdt.vertices.size())
	{
		return;
	}

	std::set<CDT::TriInd> neighbors = get_neighbors(triangulation, orig_idx, vi);

	std::vector<EdgeTriangleSurface> surface = get_surface(triangulation, neighbors, vi);

	// create other triangulation
	octopus::Triangulation other_triangulation;
	other_triangulation.init(triangulation._size_x, triangulation._size_y);

	auto added_points = triangulate(triangulation, other_triangulation, surface);
	std::vector<CDT::TriInd> new_triangles = get_triangles(other_triangulation, surface, added_points);

	// remove triangles in old triangulation
	CDT::TriIndUSet u_set(neighbors.begin(), neighbors.end());
	CDT::TriIndUMap u_map = triangulation.cdt.removeTriangles(u_set);
	for(EdgeTriangleSurface &info : surface)
	{
		info.triangle = u_map[info.triangle];
	}

	std::unordered_map<CDT::TriInd, CDT::TriInd> map_other_to_ref_triangle_index;
	// add triangles in ref triangulation
	for(CDT::TriInd const &new_tr_idx : new_triangles)
	{
		CDT::Triangle new_triangle = other_triangulation.cdt.triangles[new_tr_idx];
		new_triangle.vertices[0] = added_points[new_triangle.vertices[0]];
		new_triangle.vertices[1] = added_points[new_triangle.vertices[1]];
		new_triangle.vertices[2] = added_points[new_triangle.vertices[2]];
		bool in_surface = false;
		// safety check against surface triangle (in case of concace shape)
		for(EdgeTriangleSurface const &info : surface)
		{
			if(triangulation.cdt.triangles[info.triangle].vertices == new_triangle.vertices)
			{
				in_surface = true;
				break;
			}
		}
		if(!in_surface)
		{
			map_other_to_ref_triangle_index[new_tr_idx] = triangulation.cdt.addTriangle(new_triangle);
		}
	}

	for(auto &&pair : map_other_to_ref_triangle_index)
	{
		CDT::TriInd new_tr_idx = pair.second;
		CDT::Triangle &triangle = triangulation.cdt.triangles[new_tr_idx];

		// index found (used to find the indexes of the unmatched neighbour)
		std::vector<Unmatched> unmatcheds;

		for(size_t i = 0 ; i < triangle.neighbors.size() ; ++ i)
		{
			// transform triangle index
			if(map_other_to_ref_triangle_index.find(triangle.neighbors[i]) != map_other_to_ref_triangle_index.end())
			{
				// use transcription index from other triangulation to reference triangulation
				triangle.neighbors[i] = map_other_to_ref_triangle_index[triangle.neighbors[i]];
			}
			else
			{
				Unmatched unmatched {i};
				// triangle from other triangulation
				CDT::Triangle const &other_triangle = other_triangulation.cdt.triangles[triangle.neighbors[i]];
				unmatched.edge = get_contact_edge(other_triangle, triangle, added_points);
				unmatcheds.push_back(unmatched);
			}
		}
		for(Unmatched const &unmatched : unmatcheds)
		{
			// find surface info corresponding to edge
			for(EdgeTriangleSurface const &info : surface)
			{
				// if matching edge
				if(info.edge[0] == unmatched.edge[0] && info.edge[1] == unmatched.edge[1])
				{
					// update neighbour
					triangle.neighbors[unmatched.idx] = info.triangle;
					triangulation.cdt.triangles[info.triangle].neighbors[info.neighbour_idx] = new_tr_idx;
				}
			}
		}
	}
}

} // octopus
