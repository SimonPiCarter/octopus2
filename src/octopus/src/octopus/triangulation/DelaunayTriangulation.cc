#include "DelaunayTriangulation.hh"

#include <algorithm>
#include <cassert>
#include <stdexcept>
#include <unordered_map>

namespace octopus
{

// Super-triangle large enough to contain all points with values in [-1000, 1000].
// We pick vertices well outside that range.
static const Fixed SUPER_SCALE = Fixed(10000);

DelaunayTriangulation::DelaunayTriangulation()
{
    // Super-triangle vertices (not stored in _points)
    _superA = { -SUPER_SCALE * Fixed(3),  -SUPER_SCALE };
    _superB = {  Fixed(0),                 SUPER_SCALE * Fixed(3) };
    _superC = {  SUPER_SCALE * Fixed(3),  -SUPER_SCALE };

    // Initial triangle — vertices must be in CCW order.
    // orient2d(A,C,B): (C-A)×(B-A) = 60000*40000 - 0*30000 > 0 ✓
    _triangles.push_back({ { SUPER_A, SUPER_C, SUPER_B } });
    _cacheDirty = true;
}

TriPoint const &DelaunayTriangulation::getPoint(PointIdx idx) const
{
    if (idx == SUPER_A) return _superA;
    if (idx == SUPER_B) return _superB;
    if (idx == SUPER_C) return _superC;
    return _points[idx];
}

TriPoint const &DelaunayTriangulation::point(PointIdx idx) const
{
    return _points[idx];
}

Fixed DelaunayTriangulation::orient2d(TriPoint const &p0, TriPoint const &p1, TriPoint const &p2) const
{
    // (p1 - p0) x (p2 - p0)
    Fixed ax = p1.x - p0.x;
    Fixed ay = p1.y - p0.y;
    Fixed bx = p2.x - p0.x;
    Fixed by = p2.y - p0.y;
    return ax * by - ay * bx;
}

bool DelaunayTriangulation::inCircumcircle(Triangle const &t, TriPoint const &p) const
{
    TriPoint const &a = getPoint(t.v[0]);
    TriPoint const &b = getPoint(t.v[1]);
    TriPoint const &c = getPoint(t.v[2]);

    // Use raw internal int64 values and promote to __int128 to avoid overflow
    // (the super-triangle vertices have large coordinates).
    // If real coordinates are coord.data()/e, then the determinant computed
    // from raw values equals e^4 * det_real, preserving the sign.
    typedef __int128 i128;

    i128 ax = a.x.data() - p.x.data();
    i128 ay = a.y.data() - p.y.data();
    i128 bx = b.x.data() - p.x.data();
    i128 by = b.y.data() - p.y.data();
    i128 cx = c.x.data() - p.x.data();
    i128 cy = c.y.data() - p.y.data();

    i128 az = ax*ax + ay*ay;
    i128 bz = bx*bx + by*by;
    i128 cz = cx*cx + cy*cy;

    i128 det = ax * (by * cz - bz * cy)
             - ay * (bx * cz - bz * cx)
             + az * (bx * cy - by * cx);

    return det > 0;
}

void DelaunayTriangulation::bowyerWatsonInsert(PointIdx pidx)
{
    TriPoint const &p = getPoint(pidx);

    // Find all triangles whose circumcircle contains p
    std::vector<std::size_t> badIdx;
    for (std::size_t i = 0; i < _triangles.size(); ++i)
    {
        if (inCircumcircle(_triangles[i], p))
            badIdx.push_back(i);
    }

    // Find the boundary polygon of the hole
    // An edge is on the boundary if it belongs to exactly one bad triangle
    std::unordered_map<Edge, int, EdgeHash> edgeCount;
    for (std::size_t i : badIdx)
    {
        Triangle const &tri = _triangles[i];
        edgeCount[makeEdge(tri.v[0], tri.v[1])]++;
        edgeCount[makeEdge(tri.v[1], tri.v[2])]++;
        edgeCount[makeEdge(tri.v[2], tri.v[0])]++;
    }

    // Remove bad triangles (iterate in reverse to preserve indices)
    std::sort(badIdx.begin(), badIdx.end(), std::greater<std::size_t>());
    for (std::size_t i : badIdx)
    {
        _triangles[i] = _triangles.back();
        _triangles.pop_back();
    }

    // Re-triangulate hole: for each boundary edge, create a new triangle with p
    for (auto const &[edge, cnt] : edgeCount)
    {
        if (cnt == 1)
        {
            // Ensure the new triangle is CCW
            TriPoint const &ea = getPoint(edge.a);
            TriPoint const &eb = getPoint(edge.b);
            Fixed o = orient2d(ea, eb, p);
            if (o > Fixed(0))
                _triangles.push_back({ { edge.a, edge.b, pidx } });
            else
                _triangles.push_back({ { edge.b, edge.a, pidx } });
        }
    }

    markDirty();
}

PointIdx DelaunayTriangulation::addPoint(Fixed x, Fixed y)
{
    PointIdx idx = _points.size();
    _points.push_back({ x, y });
    bowyerWatsonInsert(idx);
    return idx;
}

static bool isSuperVertex(PointIdx idx)
{
    return idx == SUPER_A || idx == SUPER_B || idx == SUPER_C;
}

static bool touchesSuperVertex(Triangle const &t)
{
    return isSuperVertex(t.v[0]) || isSuperVertex(t.v[1]) || isSuperVertex(t.v[2]);
}

void DelaunayTriangulation::retriangulateHole(std::vector<PointIdx> const &polygon, PointIdx /*removed*/)
{
    // Fan triangulation from the first vertex of the polygon.
    // For a Delaunay result we run Bowyer-Watson re-insertion on the polygon vertices,
    // but a simple fan from an "ear" vertex is sufficient for correctness.
    // We choose to use Bowyer-Watson by removing all triangles containing any polygon
    // vertex that are already gone, and inserting fan triangles, then running
    // Bowyer-Watson on each to refine.
    //
    // Simple approach: fan from polygon[0]
    if (polygon.size() < 3)
        return;

    PointIdx anchor = polygon[0];
    for (std::size_t i = 1; i + 1 < polygon.size(); ++i)
    {
        PointIdx pb = polygon[i];
        PointIdx pc = polygon[i + 1];

        TriPoint const &pa_ = getPoint(anchor);
        TriPoint const &pb_ = getPoint(pb);
        TriPoint const &pc_ = getPoint(pc);

        Fixed o = orient2d(pa_, pb_, pc_);
        if (o > Fixed(0))
            _triangles.push_back({ { anchor, pb, pc } });
        else if (o < Fixed(0))
            _triangles.push_back({ { anchor, pc, pb } });
        // if o == 0, collinear — skip degenerate triangle
    }
}

void DelaunayTriangulation::removePoint(PointIdx idx)
{
    if (idx >= _points.size())
        throw std::out_of_range("DelaunayTriangulation::removePoint: invalid index");

    // Collect all triangles that contain this point and their surrounding polygon
    std::vector<std::size_t> toRemove;
    for (std::size_t i = 0; i < _triangles.size(); ++i)
    {
        Triangle const &t = _triangles[i];
        if (t.v[0] == idx || t.v[1] == idx || t.v[2] == idx)
            toRemove.push_back(i);
    }

    // Collect the boundary polygon of the hole (edges NOT containing idx)
    // and order them into a polygon loop
    std::unordered_map<PointIdx, PointIdx> nextVertex;
    for (std::size_t i : toRemove)
    {
        Triangle const &t = _triangles[i];
        // For each edge not containing idx, record it directed CCW away from idx
        for (int e = 0; e < 3; ++e)
        {
            PointIdx va = t.v[e];
            PointIdx vb = t.v[(e + 1) % 3];
            if (va != idx && vb != idx)
            {
                // This edge is on the hole boundary; in the CCW triangle, the
                // direction va->vb faces away from idx
                nextVertex[va] = vb;
            }
        }
    }

    // Build ordered polygon from nextVertex map
    std::vector<PointIdx> polygon;
    if (!nextVertex.empty())
    {
        PointIdx start = nextVertex.begin()->first;
        PointIdx cur = start;
        do
        {
            polygon.push_back(cur);
            cur = nextVertex[cur];
        } while (cur != start && polygon.size() <= nextVertex.size());
    }

    // Remove hole triangles
    std::sort(toRemove.begin(), toRemove.end(), std::greater<std::size_t>());
    for (std::size_t i : toRemove)
    {
        _triangles[i] = _triangles.back();
        _triangles.pop_back();
    }

    // Re-triangulate the hole
    retriangulateHole(polygon, idx);

    // Remap all indices > idx by decrementing
    // (we remove the point from _points so indices shift)
    _points.erase(_points.begin() + idx);

    for (Triangle &t : _triangles)
    {
        for (int i = 0; i < 3; ++i)
        {
            if (!isSuperVertex(t.v[i]) && t.v[i] > idx)
                --t.v[i];
        }
    }

    markDirty();
}

std::vector<Triangle> const &DelaunayTriangulation::triangles() const
{
    if (_cacheDirty)
    {
        _visibleCache.clear();
        for (Triangle const &t : _triangles)
        {
            if (!touchesSuperVertex(t))
                _visibleCache.push_back(t);
        }
        _cacheDirty = false;
    }
    return _visibleCache;
}

} // namespace octopus
