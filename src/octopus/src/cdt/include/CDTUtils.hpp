/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/**
 * @file
 * Utilities and helpers - implementation
 */

#include "CDTUtils.h"

#include "predicates.h" // robust predicates: orient, in-circle

#include <stdexcept>

namespace CDT
{

//*****************************************************************************
// V2d
//*****************************************************************************
template <typename T>
V2d<T> V2d<T>::make(const T x, const T y)
{
    V2d<T> out = {x, y};
    return out;
}

//*****************************************************************************
// Box2d
//*****************************************************************************
template <typename T>
Box2d<T> envelopBox(const std::vector<V2d<T> >& vertices)
{
    return envelopBox<T>(
        vertices.begin(), vertices.end(), getX_V2d<T>, getY_V2d<T>);
}

//*****************************************************************************
// Edge
//*****************************************************************************
CDT_INLINE_IF_HEADER_ONLY Edge::Edge(VertInd iV1, VertInd iV2)
    : m_vertices(
          iV1 < iV2 ? std::make_pair(iV1, iV2) : std::make_pair(iV2, iV1))
{}

CDT_INLINE_IF_HEADER_ONLY bool Edge::operator==(const Edge& other) const
{
    return m_vertices == other.m_vertices;
}

CDT_INLINE_IF_HEADER_ONLY bool Edge::operator!=(const Edge& other) const
{
    return !(this->operator==(other));
}

CDT_INLINE_IF_HEADER_ONLY VertInd Edge::v1() const
{
    return m_vertices.first;
}

CDT_INLINE_IF_HEADER_ONLY VertInd Edge::v2() const
{
    return m_vertices.second;
}

CDT_INLINE_IF_HEADER_ONLY const std::pair<VertInd, VertInd>& Edge::verts() const
{
    return m_vertices;
}

//*****************************************************************************
// Utility functions
//*****************************************************************************
CDT_INLINE_IF_HEADER_ONLY Index ccw(Index i)
{
    return Index((i + 1) % 3);
}

CDT_INLINE_IF_HEADER_ONLY Index cw(Index i)
{
    return Index((i + 2) % 3);
}

CDT_INLINE_IF_HEADER_ONLY bool isOnEdge(const PtTriLocation::Enum location)
{
    return location == PtTriLocation::OnEdge1 ||
           location == PtTriLocation::OnEdge2 ||
           location == PtTriLocation::OnEdge3;
}

CDT_INLINE_IF_HEADER_ONLY Index edgeNeighbor(const PtTriLocation::Enum location)
{
    assert(isOnEdge(location));
    return static_cast<Index>(location - PtTriLocation::OnEdge1);
}

template <typename T>
T orient2D(const V2d<T>& p, const V2d<T>& v1, const V2d<T>& v2)
{
    return predicates::adaptive::orient2d(v1.x, v1.y, v2.x, v2.y, p.x, p.y);
}

template <typename T>
PtLineLocation::Enum locatePointLine(
    const V2d<T>& p,
    const V2d<T>& v1,
    const V2d<T>& v2,
    const T orientationTolerance)
{
    return classifyOrientation(orient2D(p, v1, v2), orientationTolerance);
}

template <typename T>
PtLineLocation::Enum
classifyOrientation(const T orientation, const T orientationTolerance)
{
    if(orientation < -orientationTolerance)
        return PtLineLocation::Right;
    if(orientation > orientationTolerance)
        return PtLineLocation::Left;
    return PtLineLocation::OnLine;
}

template <typename T>
PtTriLocation::Enum locatePointTriangle(
    const V2d<T>& p,
    const V2d<T>& v1,
    const V2d<T>& v2,
    const V2d<T>& v3)
{
    using namespace predicates::adaptive;
    PtTriLocation::Enum result = PtTriLocation::Inside;
    PtLineLocation::Enum edgeCheck = locatePointLine(p, v1, v2);
    if(edgeCheck == PtLineLocation::Right)
        return PtTriLocation::Outside;
    if(edgeCheck == PtLineLocation::OnLine)
        result = PtTriLocation::OnEdge1;
    edgeCheck = locatePointLine(p, v2, v3);
    if(edgeCheck == PtLineLocation::Right)
        return PtTriLocation::Outside;
    if(edgeCheck == PtLineLocation::OnLine)
    {
        result = (result == PtTriLocation::Inside) ? PtTriLocation::OnEdge2
                                                   : PtTriLocation::OnVertex;
    }
    edgeCheck = locatePointLine(p, v3, v1);
    if(edgeCheck == PtLineLocation::Right)
        return PtTriLocation::Outside;
    if(edgeCheck == PtLineLocation::OnLine)
    {
        result = (result == PtTriLocation::Inside) ? PtTriLocation::OnEdge3
                                                   : PtTriLocation::OnVertex;
    }
    return result;
}

CDT_INLINE_IF_HEADER_ONLY Index opoNbr(const Index vertIndex)
{
    if(vertIndex == Index(0))
        return Index(1);
    if(vertIndex == Index(1))
        return Index(2);
    if(vertIndex == Index(2))
        return Index(0);
    assert(false && "Invalid vertex index");
    // throw std::runtime_error("Invalid vertex index");
}

CDT_INLINE_IF_HEADER_ONLY Index opoVrt(const Index neighborIndex)
{
    if(neighborIndex == Index(0))
        return Index(2);
    if(neighborIndex == Index(1))
        return Index(0);
    if(neighborIndex == Index(2))
        return Index(1);
    assert(false && "Invalid neighbor index");
    // throw std::runtime_error("Invalid neighbor index");
}

CDT_INLINE_IF_HEADER_ONLY Index
opposedTriangleInd(const VerticesArr3& vv, const VertInd iVert)
{
    assert(vv[0] == iVert || vv[1] == iVert || vv[2] == iVert);
    if(vv[0] == iVert)
        return Index(1);
    if(vv[1] == iVert)
        return Index(2);
    return Index(0);
}

CDT_INLINE_IF_HEADER_ONLY Index edgeNeighborInd(
    const VerticesArr3& vv,
    const VertInd iVedge1,
    const VertInd iVedge2)
{
    assert(vv[0] == iVedge1 || vv[1] == iVedge1 || vv[2] == iVedge1);
    assert(vv[0] == iVedge2 || vv[1] == iVedge2 || vv[2] == iVedge2);
    assert(
        (vv[0] != iVedge1 && vv[0] != iVedge2) ||
        (vv[1] != iVedge1 && vv[1] != iVedge2) ||
        (vv[2] != iVedge1 && vv[2] != iVedge2));
    /*
     *      vv[2]
     *       /\
     *  n[2]/  \n[1]
     *     /____\
     * vv[0] n[0] vv[1]
     */
    if(vv[0] == iVedge1)
    {
        if(vv[1] == iVedge2)
            return Index(0);
        return Index(2);
    }
    if(vv[0] == iVedge2)
    {
        if(vv[1] == iVedge1)
            return Index(0);
        return Index(2);
    }
    return Index(1);
}

CDT_INLINE_IF_HEADER_ONLY Index
opposedVertexInd(const NeighborsArr3& nn, const TriInd iTopo)
{
    assert(nn[0] == iTopo || nn[1] == iTopo || nn[2] == iTopo);
    if(nn[0] == iTopo)
        return Index(2);
    if(nn[1] == iTopo)
        return Index(0);
    return Index(1);
}

CDT_INLINE_IF_HEADER_ONLY Index
vertexInd(const VerticesArr3& vv, const VertInd iV)
{
    assert(vv[0] == iV || vv[1] == iV || vv[2] == iV);
    if(vv[0] == iV)
        return Index(0);
    if(vv[1] == iV)
        return Index(1);
    return Index(2);
}

CDT_INLINE_IF_HEADER_ONLY TriInd
opposedTriangle(const Triangle& tri, const VertInd iVert)
{
    return tri.neighbors[opposedTriangleInd(tri.vertices, iVert)];
}

/// Given one triangles, return vertex of first triangle opposed to the second
CDT_EXPORT CDT_INLINE_IF_HEADER_ONLY VertInd
opposedVertex(const Triangle& tri, VertInd iVedge1, VertInd iVedge2)
{
    if(tri.vertices[0] != iVedge1 && tri.vertices[0] != iVedge2)
        return tri.vertices[0];
    if(tri.vertices[1] != iVedge1 && tri.vertices[1] != iVedge2)
        return tri.vertices[1];
    return tri.vertices[2];
}

CDT_INLINE_IF_HEADER_ONLY VertInd
opposedVertex(const Triangle& tri, const TriInd iTopo)
{
    return tri.vertices[opposedVertexInd(tri.neighbors, iTopo)];
}

/// Given triangle and an edge find neighbor sharing the edge
CDT_INLINE_IF_HEADER_ONLY TriInd
edgeNeighbor(const Triangle& tri, VertInd iVedge1, VertInd iVedge2)
{
    return tri.neighbors[edgeNeighborInd(tri.vertices, iVedge1, iVedge2)];
}

template <typename T>
bool isInCircumcircle(
    const V2d<T>& p,
    const V2d<T>& v1,
    const V2d<T>& v2,
    const V2d<T>& v3)
{
    using namespace predicates::adaptive;
    return incircle(v1.x, v1.y, v2.x, v2.y, v3.x, v3.y, p.x, p.y) > T(0);
}

CDT_INLINE_IF_HEADER_ONLY
bool verticesShareEdge(const TriIndVec& aTris, const TriIndVec& bTris)
{
    for(TriIndVec::const_iterator it = aTris.begin(); it != aTris.end(); ++it)
        if(std::find(bTris.begin(), bTris.end(), *it) != bTris.end())
            return true;
    return false;
}

template <typename T>
T distanceSquared(const T ax, const T ay, const T bx, const T by)
{
    const T dx = bx - ax;
    const T dy = by - ay;
    return dx * dx + dy * dy;
}

template <typename T>
T distance(const T ax, const T ay, const T bx, const T by)
{
    return std::sqrt(distanceSquared(ax, ay, bx, by));
}

template <typename T>
T distance(const V2d<T>& a, const V2d<T>& b)
{
    return distance(a.x, a.y, b.x, b.y);
}

template <typename T>
T distanceSquared(const V2d<T>& a, const V2d<T>& b)
{
    return distanceSquared(a.x, a.y, b.x, b.y);
}

template <typename T>
V2d<T> project_on_edge(const V2d<T>& p, const V2d<T>& a, const V2d<T>& b, T const &epsilon)
{
    T dx = b.x-a.x;
    T dy = b.y-a.y;
    // horizontal line
    if(dx <= epsilon && dx >= -epsilon)
    {
        return {a.x, p.y};
    }
    // vertical line
    if(dy <= epsilon && dy >= -epsilon)
    {
        return {p.x, a.y};
    }
    T tx = (p.y - a.y) / dy;
    T u = a.x + tx * dx - p.x;

    T ty = (p.x - a.x) / dx;
    T v = a.y + ty * dy - p.y;

    return {p.x + u/2, p.y + v/2};
}

template <typename T>
bool is_projection_on_edge(const V2d<T>& p, const V2d<T>& a, const V2d<T>& b)
{
    T minx = std::min(a.x, b.x);
    T miny = std::min(a.y, b.y);
    T maxx = std::max(a.x, b.x);
    T maxy = std::max(a.y, b.y);
    return p.x >= minx && p.x <= maxx && p.y >= miny && p.y <= maxy;
}

template <typename T>
bool is_on_edge(const V2d<T>& v, const V2d<T>& a, const V2d<T>& b, T const &epsilon)
{
    return is_projection_on_edge(project_on_edge(v,a,b,epsilon), a, b);
}

bool touchesSuperTriangle(const Triangle& t)
{
    return t.vertices[0] < 3 || t.vertices[1] < 3 || t.vertices[2] < 3;
}

} // namespace CDT
