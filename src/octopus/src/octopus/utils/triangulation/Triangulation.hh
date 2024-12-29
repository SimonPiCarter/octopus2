#pragma once

#ifdef GD_EXTENSION_GODOCTOPUS
	#include <godot_cpp/godot.hpp>
	#include <godot_cpp/classes/node2d.hpp>
#else
	#include "scene/2d/node_2d.h"
#endif

#include "cdt/include/CDT.h"
#include "octopus/utils/FixedPoint.hh"
#include "game/grid/GridProxy.h"

namespace octopus {

class Triangulation {
public:
	Triangulation() : cdt(CDT::detail::defaults::vertexInsertionOrder, CDT::IntersectingConstraintEdges::TryResolve, octopus::Fixed::One()/100) {}
	~Triangulation() {}

	void reset();
	void init(int x, int y);

	int insert_point(int x, int y, int forbidden);
	void insert_edge(int idx_point_1, int idx_point_2);
	void finalize();

	std::vector<std::size_t> compute_path(Vector const &orig, Vector const &dest) const;
	std::vector<Vector> compute_funnel(Vector const &orig, Vector const &dest) const;
	std::vector<Vector> compute_funnel_from_path(Vector const &orig, Vector const &dest, std::vector<std::size_t> const &path) const;

// private:
	CDT::Triangulation<octopus::Fixed> cdt;
	std::vector<std::pair<CDT::VertInd, CDT::VertInd> > edges;
	std::vector<bool> forbidden_triangles;

	int _size_x = 0;
	int _size_y = 0;

    uint64_t revision = 0;
};

struct TriangulationPtr
{
	Triangulation * ptr = nullptr;
};

}
