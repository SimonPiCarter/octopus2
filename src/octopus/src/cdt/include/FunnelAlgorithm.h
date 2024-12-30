/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include <set>
#include <vector>
// #include <core/math/vector2.h>

namespace CDT
{

template<typename T>
struct FunnelDebug
{
    int steps = 0;
    V2d<T> orig;
    V2d<T> left;
    V2d<T> right;
    V2d<T> candidate;
};

template<typename triangle>
std::pair<VertInd, VertInd> common_vertices(triangle const &tr, triangle const &other)
{
    bool common[3];
    for(size_t i = 0 ; i < 3 ; ++ i)
    {
        common[i] = tr.vertices[i] == other.vertices[0]
            || tr.vertices[i] == other.vertices[1]
            || tr.vertices[i] == other.vertices[2];
    }
    if(common[0] && common[1])
    {
        return {tr.vertices[0], tr.vertices[1]};
    }
    if(common[0] && common[2])
    {
        return {tr.vertices[0], tr.vertices[2]};
    }
    return {tr.vertices[1], tr.vertices[2]};
}

template<typename T>
std::vector<Edge> compute_portals(Triangulation<T> const &triangulation, std::vector<std::size_t> const &triangles_path)
{
    std::vector<Edge> portals;
    size_t last_idx = triangles_path[0];
    for(size_t cur_idx : triangles_path)
    {
        // skip first
        if(last_idx == cur_idx)
        {
            continue;
        }
        std::pair<VertInd, VertInd> com = common_vertices(triangulation.triangles[last_idx], triangulation.triangles[cur_idx]);

        portals.push_back({com.first, com.second});
        last_idx = cur_idx;
    }
    return portals;
}

struct FunnelPartResult
{
    VertInd point;
    size_t cur_portal = 0;
    bool over = false; // means the point is the final target
    bool no_point = false; // means the index point should be ignored
};

template<typename T>
VertInd get_left(V2d<T> const &origin_p, Triangulation<T> const &triangulation, VertInd a, VertInd b)
{
    PtLineLocation::Enum pos_a = locatePointLine(triangulation.vertices[a], origin_p, triangulation.vertices[b]);
    if(pos_a == PtLineLocation::Left)
    {
        return a;
    }
    return b;
}

inline FunnelPartResult get_next_portal(std::size_t start_portals_idx, VertInd const &start_point, std::vector<Edge> const &portals)
{
    for(size_t portal = start_portals_idx ; portal < portals.size() ; ++ portal)
    {
        Edge const &edge = portals[portal];
        if(edge.v1() != start_point && edge.v2() != start_point)
        {
            return {start_point, portal, false, false};
        }
    }
    // If we reach here it means no more portal is reached before the target
    // hence we can reach the target directly but still require the start_point to be added
    return {start_point,0,true,false};
}

template<typename T>
FunnelPartResult funnel_algorithm_part(
    V2d<T> const &origin_p,
    V2d<T> const &target_p,
    std::size_t start_portals_idx,
    Triangulation<T> const &triangulation,
    std::vector<Edge> const &portals,
    FunnelDebug<T> *debug_p,
    int &steps
)
{
    if(debug_p)
    {
        debug_p->orig = origin_p;
    }
    std::size_t cur_portals = start_portals_idx;
    Edge cur_edge = portals[cur_portals];

    VertInd left = get_left(origin_p, triangulation, cur_edge.v1(), cur_edge.v2());
    VertInd right = left == cur_edge.v1() ? cur_edge.v2() : cur_edge.v1();
    bool left_stuck = false;
    bool right_stuck = false;
    std::size_t stuck_portal = cur_portals;
    std::size_t left_portal = cur_portals;
    std::size_t right_portal = cur_portals;

    while(cur_portals+1 < portals.size())
    {
        Edge next_edge = portals[cur_portals+1];
        VertInd candidate_left = get_left(origin_p, triangulation, next_edge.v1(), next_edge.v2());
        VertInd candidate_right = candidate_left == next_edge.v1() ? next_edge.v2() : next_edge.v1();

        if(debug_p)
        {
            debug_p->left = triangulation.vertices[left];
            debug_p->right = triangulation.vertices[right];
            debug_p->candidate = triangulation.vertices[candidate_left];

            ++steps;
            if(steps >= debug_p->steps) { return {0,0, true, true}; }
        }

        if(locatePointLine(triangulation.vertices[candidate_right], origin_p, triangulation.vertices[left]) == PtLineLocation::OnLine)
        {
            return get_next_portal(left_portal, left, portals);
        }

        if(locatePointLine(triangulation.vertices[candidate_left], origin_p, triangulation.vertices[right]) == PtLineLocation::OnLine)
        {
            return get_next_portal(right_portal, right, portals);
        }

        if(candidate_left != left)
        {
            PtLineLocation::Enum pos_relative_to_right = locatePointLine(triangulation.vertices[candidate_left], origin_p, triangulation.vertices[right]);
            PtLineLocation::Enum pos_relative_to_left = locatePointLine(triangulation.vertices[candidate_left], origin_p, triangulation.vertices[left]);

            // new point in the funnel
            if(pos_relative_to_right != PtLineLocation::Right
            && pos_relative_to_left != PtLineLocation::Left)
            {
                left = candidate_left;
                left_portal = cur_portals+1;
            }
            else if(pos_relative_to_right == PtLineLocation::Right)
            {
                // new funnel
                return get_next_portal(right_portal, right, portals);
            }
            // Harden left and remember portal
            else
            {
                stuck_portal = cur_portals;
                left_stuck = true;
            }
        }

        if(debug_p)
        {
            debug_p->left = triangulation.vertices[left];
            debug_p->right = triangulation.vertices[right];
            debug_p->candidate = triangulation.vertices[candidate_right];

            ++steps;
            if(steps >= debug_p->steps) { return {0,0, true, true}; }
        }

        if(candidate_right != right)
        {
            PtLineLocation::Enum pos_relative_to_right = locatePointLine(triangulation.vertices[candidate_right], origin_p, triangulation.vertices[right]);
            PtLineLocation::Enum pos_relative_to_left = locatePointLine(triangulation.vertices[candidate_right], origin_p, triangulation.vertices[left]);

            // new point in the funnel
            if(pos_relative_to_right != PtLineLocation::Right
            && pos_relative_to_left != PtLineLocation::Left)
            {
                right = candidate_right;
                right_portal = cur_portals+1;
            }
            else if(pos_relative_to_left == PtLineLocation::Left)
            {
                // new funnel
                return get_next_portal(left_portal, left, portals);
            }
            // Harden right and remember portal
            else
            {
                stuck_portal = cur_portals;
                right_stuck = true;
            }
        }
        ++cur_portals;
    }

    PtLineLocation::Enum pos_relative_to_right = locatePointLine(target_p, origin_p, triangulation.vertices[right]);
    PtLineLocation::Enum pos_relative_to_left = locatePointLine(target_p, origin_p, triangulation.vertices[left]);
    // try left moving to the target
    if(right_stuck)
    {
        // new point in the funnel
        if(pos_relative_to_right != PtLineLocation::Right
        && pos_relative_to_left != PtLineLocation::Left)
        {
            // over and no need to add new point
            return {0,0,true, true};
        }
        else if(pos_relative_to_right == PtLineLocation::Right)
        {
            // new funnel
            return get_next_portal(right_portal, right, portals);
        }
        // Harden left and remember portal
        else
        {
            return get_next_portal(left_portal, left, portals);
        }
    }

    if(left_stuck)
    {
        // new point in the funnel
        if(pos_relative_to_right != PtLineLocation::Right
        && pos_relative_to_left != PtLineLocation::Left)
        {
            // over and no need to add new point
            return {0,0,true, true};
        }
        else if(pos_relative_to_left == PtLineLocation::Left)
        {
            // new funnel
            return get_next_portal(left_portal, left, portals);
        }
        // Harden right and remember portal
        else
        {
            return get_next_portal(right_portal, right, portals);
        }
    }

    // Handling last portal when not stuck

    // new point in the funnel
    if(pos_relative_to_right != PtLineLocation::Right
    && pos_relative_to_left != PtLineLocation::Left)
    {
        // over and no need to add new point
        return {0,0,true, true};
    }
    else if (pos_relative_to_right == PtLineLocation::Right)
    {
        // over but need to add right point
        return {right,0,true, false};
    }

    // over but need to add left point
    return {left,0,true, false};
}

template<typename T>
std::vector<V2d<T>> funnel_algorithm(Triangulation<T> const &triangulation, std::vector<std::size_t> const &triangles_path,
    V2d<T> const &origin_p,
    V2d<T> const &target_p,
    FunnelDebug<T> *debug_p=nullptr
)
{
    std::vector<V2d<T>> path_l = {origin_p};
    int steps = 0;

    auto && portals = compute_portals(triangulation, triangles_path);

    FunnelPartResult result_l;
    // We are done
    result_l.over = portals.empty();
    V2d<T> cur_point = origin_p;

    while(!result_l.over)
    {
        result_l = funnel_algorithm_part(cur_point, target_p, result_l.cur_portal, triangulation, portals, debug_p, steps);
        if(!result_l.no_point)
        {
            cur_point = triangulation.vertices[result_l.point];
            path_l.push_back(cur_point);
        }
    }

    path_l.push_back(target_p);
    return path_l;
}

}