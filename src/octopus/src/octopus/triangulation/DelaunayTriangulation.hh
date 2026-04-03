#ifndef __Octopus_DelaunayTriangulation__
#define __Octopus_DelaunayTriangulation__

#include <array>
#include <cstddef>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "octopus/utils/FixedPoint.hh"

namespace octopus
{

struct TriPoint
{
    Fixed x;
    Fixed y;

    bool operator==(TriPoint const &o) const { return x == o.x && y == o.y; }
    bool operator!=(TriPoint const &o) const { return !(*this == o); }
};

/// Index into Triangulation::_points (large sentinel values for base vertices)
using PointIdx = std::size_t;

static constexpr PointIdx BASE_BL = std::size_t(-1); ///< (-1001,-1001)
static constexpr PointIdx BASE_BR = std::size_t(-2); ///< ( 1001,-1001)
static constexpr PointIdx BASE_TR = std::size_t(-3); ///< ( 1001, 1001)
static constexpr PointIdx BASE_TL = std::size_t(-4); ///< (-1001, 1001)

struct Triangle
{
    std::array<PointIdx, 3> v; ///< vertex indices, CCW order
};

/// Edge represented as an ordered pair of indices (smaller first)
struct Edge
{
    PointIdx a, b;
    bool operator==(Edge const &o) const { return a == o.a && b == o.b; }
};

struct EdgeHash
{
    std::size_t operator()(Edge const &e) const
    {
        // combine the two indices
        std::size_t h1 = std::hash<std::size_t>{}(e.a);
        std::size_t h2 = std::hash<std::size_t>{}(e.b);
        return h1 ^ (h2 * 2654435761ULL);
    }
};

/// Make a canonical (unordered) edge
inline Edge makeEdge(PointIdx a, PointIdx b) { return a < b ? Edge{a, b} : Edge{b, a}; }

/// Bowyer-Watson Delaunay triangulation supporting dynamic insertion and removal.
/// Uses two bounding triangles covering [-1001,1001]² to bootstrap the triangulation;
/// base vertices are excluded from the final triangulation queries.
class DelaunayTriangulation
{
public:
    DelaunayTriangulation();

    /// Insert a new point. Returns the index of the point.
    PointIdx addPoint(Fixed x, Fixed y);

    /// Remove a point by index. The point must have been previously inserted.
    void removePoint(PointIdx idx);

    /// Access user-visible triangles (those not touching base vertices).
    std::vector<Triangle> const &triangles() const;

    /// Access point by index.
    TriPoint const &point(PointIdx idx) const;

    /// Number of user points (excluding base vertices).
    std::size_t pointCount() const { return _points.size(); }

private:
    std::vector<TriPoint> _points; ///< user points indexed by PointIdx
    TriPoint _baseBL, _baseBR, _baseTR, _baseTL; ///< bounding-box corner vertices

    std::vector<Triangle> _triangles; ///< all triangles including base-vertex ones
    mutable std::vector<Triangle> _visibleCache;
    mutable bool _cacheDirty = true;

    TriPoint const &getPoint(PointIdx idx) const;

    /// Circumcircle test: is point p strictly inside the circumcircle of triangle t?
    bool inCircumcircle(Triangle const &t, TriPoint const &p) const;

    /// Orient2d sign: positive if p0,p1,p2 are CCW
    Fixed orient2d(TriPoint const &p0, TriPoint const &p1, TriPoint const &p2) const;

    /// Bowyer-Watson insertion step (works with raw index, including base vertices)
    void bowyerWatsonInsert(PointIdx pidx);

    /// Re-triangulate the hole left by removing a point using fan triangulation
    /// of the surrounding polygon.
    void retriangulateHole(std::vector<PointIdx> const &polygon, PointIdx removed);

    void markDirty() { _cacheDirty = true; }
};

} // namespace octopus

#endif // __Octopus_DelaunayTriangulation__
