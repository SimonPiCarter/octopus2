#ifndef __Octopus_DelaunayTriangulation__
#define __Octopus_DelaunayTriangulation__

#include <array>
#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "octopus/utils/FixedPoint.hh"

namespace octopus
{

static constexpr long long TriPointScale = 100;

inline long long to_tri_coord(Fixed const &p)
{
    return p.data() / (Fixed::OneAsLong() / TriPointScale);
}

inline Fixed to_fixed(long long p)
{
    return Fixed(p) / TriPointScale;
}

struct TriPoint
{
    long long x; ///< X coordinate in hundredths of a world unit
    long long y; ///< Y coordinate in hundredths of a world unit

    bool operator==(TriPoint const &o) const { return x == o.x && y == o.y; }
    bool operator!=(TriPoint const &o) const { return !(*this == o); }
};

/// Index into Triangulation::_points (large sentinel values for base vertices)
using PointIdx = std::size_t;

static constexpr PointIdx BASE_A = std::size_t(-1); ///< (-4000,-2000) super-triangle vertex A
static constexpr PointIdx BASE_B = std::size_t(-2); ///< ( 4000,-2000) super-triangle vertex B
static constexpr PointIdx BASE_C = std::size_t(-3); ///< (    0, 4000) super-triangle vertex C

struct Triangle
{
    std::array<PointIdx, 3> v; ///< vertex indices, CCW order
    bool hole = false;         ///< true if this triangle is inside a hole region
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
/// Uses a single super-triangle covering [-1000,1000]² to bootstrap the triangulation;
/// base vertices are excluded from the final triangulation queries.
class DelaunayTriangulation
{
public:
    DelaunayTriangulation();

    /// Insert a new point, or return the existing index if its coordinates are already present.
    PointIdx addPoint(Fixed x, Fixed y);

    /// Remove a point by index. The point must have been previously inserted
    /// and must not participate in any constrained edge (both conditions are asserted).
    void removePoint(PointIdx idx);

    /// Remove points by their indices before the batch starts. Indices are
    /// processed in descending order; duplicate indices are ignored.
    /// None of the points may participate in a constrained edge.
    void removePoints(std::vector<PointIdx> const &indices);

    /// Access user-visible triangles (those not touching base vertices and not inside holes).
    std::vector<Triangle> const &triangles() const;

    /// Access hole triangles (those not touching base vertices but inside hole regions).
    std::vector<Triangle> const &holeTriangles() const;

    /// Access point by index.
    TriPoint const &point(PointIdx idx) const;

    /// Number of user points (excluding base vertices).
    std::size_t pointCount() const { return _points.size(); }

    /// Revision of the triangulation state used by navigation.
    std::uint64_t revision() const { return _revision; }

    // ── Constrained edges ────────────────────────────────────────────────────

    /// Insert a constraint edge between two existing points. The edge will be
    /// preserved through subsequent point insertions. If the edge does not yet
    /// exist in the triangulation it is forced in by re-triangulating the
    /// crossing region.
    void addConstrainedEdge(PointIdx a, PointIdx b);

    /// Un-constrain an edge. The edge may still be present in the triangulation
    /// but will no longer be protected from future operations.
    void removeConstrainedEdge(PointIdx a, PointIdx b);

    /// Return true if the edge (a,b) is currently constrained.
    bool isConstrained(PointIdx a, PointIdx b) const;

    /// Return true if the point participates in any constrained edge.
    bool isPointConstrained(PointIdx idx) const;

    // ── Holes ────────────────────────────────────────────────────────────────

    /// Mark all triangles strictly inside the given closed polygon as holes.
    /// The polygon vertices must already be in the triangulation and are given
    /// in order (CCW or CW; the implementation determines the inside).
    /// Each polygon edge is automatically added as a constrained edge.
    void markHole(std::vector<PointIdx> const &polygon);

    /// Remove one polygon's hole flag while leaving its points and constraints
    /// untouched. Callers that replace or delete a footprint must release its
    /// boundary constraints before removing its points.
    void removeHole(std::vector<PointIdx> const &polygon);

    /// Reset all hole flags and remove all constrained edges.
    void clearHoles();

private:
    std::vector<TriPoint> _points; ///< user points indexed by PointIdx
    TriPoint _baseA, _baseB, _baseC; ///< super-triangle vertices

    std::vector<Triangle> _triangles; ///< all triangles including base-vertex ones
    mutable std::vector<Triangle> _visibleCache;
    mutable std::vector<Triangle> _holeCache;
    mutable bool _cacheDirty = true;
    std::uint64_t _revision = 0;

    std::unordered_set<Edge, EdgeHash> _constrainedEdges; ///< edges that must not be crossed
    std::vector<std::vector<PointIdx>> _holePolygons;

    TriPoint const &getPoint(PointIdx idx) const;
    void markHoleTriangles(std::vector<PointIdx> const &polygon);
    void rebuildHoleFlags();

    /// Circumcircle test: is point p strictly inside the circumcircle of triangle t?
    bool inCircumcircle(Triangle const &t, TriPoint const &p) const;

    /// Orient2d sign: positive if p0,p1,p2 are CCW
    long long orient2d(TriPoint const &p0, TriPoint const &p1, TriPoint const &p2) const;

    /// Return true if segments (p1,p2) and (p3,p4) properly intersect.
    bool segmentsIntersect(TriPoint const &p1, TriPoint const &p2,
                           TriPoint const &p3, TriPoint const &p4) const;

    /// Bowyer-Watson insertion step using BFS flood-fill that stops at constrained edges.
    void bowyerWatsonInsert(PointIdx pidx);

    /// Find all triangle indices whose interior is crossed by segment (a,b).
    /// Also collects the ordered boundary vertices on each side of the segment.
    void walkSegment(PointIdx a, PointIdx b,
                     std::vector<std::size_t> &crossingTriangles,
                     std::vector<PointIdx> &leftPoly,
                     std::vector<PointIdx> &rightPoly) const;

    /// Return the indices of points strictly between a and b that lie exactly
    /// on segment (a,b), ordered by increasing distance from a. Used to split
    /// a requested constrained edge into sub-segments so that both
    /// addConstrainedEdge and removeConstrainedEdge agree on which edges are
    /// actually stored in _constrainedEdges.
    std::vector<PointIdx> collinearIntermediatePoints(PointIdx a, PointIdx b) const;

    /// Triangulate a simple polygon that has edge (edgeA,edgeB) as one side.
    /// polygon is ordered and must already include edgeA and edgeB as consecutive entries.
    void retriangulatePolygon(std::vector<PointIdx> const &polygon,
                              PointIdx edgeA, PointIdx edgeB);

    /// Re-triangulate the hole left by removing a point using fan triangulation
    /// of the surrounding polygon.
    void retriangulateHole(std::vector<PointIdx> const &polygon, PointIdx removed);

    void markDirty() {
        _cacheDirty = true;
        ++_revision;
    }
};

} // namespace octopus

#endif // __Octopus_DelaunayTriangulation__
