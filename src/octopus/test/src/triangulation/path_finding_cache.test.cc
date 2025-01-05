#include <gtest/gtest.h>
#include "octopus/utils/triangulation/Triangulation.hh"
#include "octopus/world/path/PathFindingCache.hh"
#include "octopus/world/stats/TimeStats.hh"
#include "octopus/systems/Systems.hh"

#include "flecs.h"

using namespace octopus;

TEST(path_finding_cache, only_triangulation)
{
    Triangulation triangulation;

    triangulation.init(500, 500);
    insert_box(triangulation, 20, 20, 20, 20, true);
    triangulation.finalize();

    std::vector<std::size_t> path = triangulation.compute_path({10, 30}, {50, 30});
    std::vector<Vector> funnel = triangulation.compute_funnel_from_path({10, 30}, {50, 30}, path);

    ASSERT_EQ(4u, funnel.size());
    EXPECT_EQ(Vector(10,30), funnel[0]);
    EXPECT_EQ(Vector(20,20), funnel[1]);
    EXPECT_EQ(Vector(40,20), funnel[2]);
    EXPECT_EQ(Vector(50,30), funnel[3]);
}

TEST(path_finding_cache, basic_query_system)
{
    TimeStats stats;
    Triangulation triangulation;
    flecs::world ecs;
    ecs.add<PathFindingCache>();
    ecs.set<TriangulationPtr>(TriangulationPtr {&triangulation});
    ecs.set<TimeStatsPtr>(TimeStatsPtr {&stats});

    set_up_phases(ecs);

    triangulation.init(500, 500);
    insert_box(triangulation, 20, 20, 20, 20, true);
    triangulation.finalize();

    // Position
    Position pos {{10,30}};

    ecs.get_mut<PathFindingCache>()->declare_cache_update_system(ecs, triangulation, stats);

    ecs.progress();

    PathQuery query = ecs.get<PathFindingCache>()->query_path(pos, {50,30});

    EXPECT_FALSE(query.is_valid());

    ecs.progress();

    ASSERT_TRUE(query.is_valid());

    Vector direction_1 = query.get_direction();
    EXPECT_EQ(Vector(10, -10), direction_1);

    pos.pos += direction_1;

    query = ecs.get<PathFindingCache>()->query_path(pos, {50,30});

    ASSERT_TRUE(query.is_valid());

    Vector direction_2 = query.get_direction();
    EXPECT_EQ(Vector(20, 0), direction_2);

    pos.pos += direction_2;

    query = ecs.get<PathFindingCache>()->query_path(pos, {50,30});

    ASSERT_TRUE(query.is_valid());

    Vector direction_3 = query.get_direction();
    EXPECT_EQ(Vector(10, 10), direction_3);

    std::cout<<"path_finding : "<<stats.path_finding<<"ms"<<std::endl;
    std::cout<<"path_funnelling : "<<stats.path_funnelling<<"ms"<<std::endl;
}
