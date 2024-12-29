/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "Triangulation.hh"

#include <algorithm>
#include <chrono>

#include "cdt/include/ShortestPath.h"
#include "cdt/include/FunnelAlgorithm.h"

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

	cdt.insertVertices({{0,0},{x,0},{x,y},{0,y},
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
	int orig_idx = -1;
	int dest_idx = -1;
	int idx_l = 0;

	for(CDT::Triangle const &tr : cdt.triangles)
	{
		if(CDT::locatePointTriangle({orig.x,orig.y},
			cdt.vertices[tr.vertices[0]],
			cdt.vertices[tr.vertices[1]],
			cdt.vertices[tr.vertices[2]]) != CDT::PtTriLocation::Outside )
		{
			orig_idx = idx_l;
		}
		if(CDT::locatePointTriangle({dest.x,dest.y},
			cdt.vertices[tr.vertices[0]],
			cdt.vertices[tr.vertices[1]],
			cdt.vertices[tr.vertices[2]]) != CDT::PtTriLocation::Outside )
		{
			dest_idx = idx_l;
		}

		if(orig_idx >= 0 && dest_idx >= 0)
		{
			break;
		}

		++idx_l;
	}

	if(orig_idx < 0 || dest_idx < 0)
	{
		return {};
	}

	return CDT::shortest_path(cdt, orig_idx, dest_idx, forbidden_triangles);
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

} // octopus
