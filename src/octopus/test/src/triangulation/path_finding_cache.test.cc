#include <gtest/gtest.h>
#include "octopus/utils/triangulation/Triangulation.hh"
#include "octopus/triangulation/DelaunayPathFindingCache.hh"
#include "octopus/triangulation/DelaunayTriangulationNavigator.hh"
#include "octopus/world/path/PathFindingCache.hh"
#include "octopus/world/path/direction.hh"
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

TEST(path_finding_cache, delaunay_navigator_uses_caller_built_mesh)
{
    DelaunayTriangulation mesh;
    std::vector<PointIdx> const boundary = {
        mesh.addPoint(Fixed(-100), Fixed(-100)),
        mesh.addPoint(Fixed(600), Fixed(-100)),
        mesh.addPoint(Fixed(600), Fixed(600)),
        mesh.addPoint(Fixed(-100), Fixed(600))
    };
    for (std::size_t i = 0; i < boundary.size(); ++i)
        mesh.addConstrainedEdge(boundary[i], boundary[(i + 1) % boundary.size()]);

    std::vector<PointIdx> const obstacle = {
        mesh.addPoint(Fixed(20), Fixed(20)),
        mesh.addPoint(Fixed(40), Fixed(20)),
        mesh.addPoint(Fixed(40), Fixed(40)),
        mesh.addPoint(Fixed(20), Fixed(40))
    };
    mesh.markHole(obstacle);

    DelaunayTriangulationNavigator navigator(mesh);
    std::vector<Vector> const funnel = navigator.compute_funnel({10, 30}, {50, 30});
    ASSERT_EQ(4u, funnel.size());
    EXPECT_EQ(Vector(10, 30), funnel[0]);
    EXPECT_EQ(Vector(20, 20), funnel[1]);
    EXPECT_EQ(Vector(40, 20), funnel[2]);
    EXPECT_EQ(Vector(50, 30), funnel[3]);
    EXPECT_TRUE(navigator.compute_path({10, 30}, {50, 30}).size() > 1);
    EXPECT_TRUE(navigator.compute_path({10, 30}, {30, 30}).empty());
    EXPECT_TRUE(navigator.compute_path_from_idx(9999, 0).empty());
    EXPECT_EQ(1, navigator.debug_funnel({10, 30}, {50, 30}, 1).steps);
}

TEST(path_finding_cache, delaunay_cache_defers_and_drives_direction)
{
    DelaunayTriangulation mesh;
    std::vector<PointIdx> const boundary = {
        mesh.addPoint(Fixed(-100), Fixed(-100)),
        mesh.addPoint(Fixed(600), Fixed(-100)),
        mesh.addPoint(Fixed(600), Fixed(600)),
        mesh.addPoint(Fixed(-100), Fixed(600))
    };
    for (std::size_t i = 0; i < boundary.size(); ++i)
        mesh.addConstrainedEdge(boundary[i], boundary[(i + 1) % boundary.size()]);

    std::vector<PointIdx> const obstacle = {
        mesh.addPoint(Fixed(20), Fixed(20)),
        mesh.addPoint(Fixed(40), Fixed(20)),
        mesh.addPoint(Fixed(40), Fixed(40)),
        mesh.addPoint(Fixed(20), Fixed(40))
    };
    mesh.markHole(obstacle);

    DelaunayTriangulationNavigator navigator(mesh);
    DelaunayPathFindingCache cache(navigator);
    TimeStats stats;
    flecs::world ecs;
    set_up_phases(ecs);
    cache.declare_cache_update_system(ecs, stats);

    Position pos {{10, 30}};
    DelaunayPathQuery query = cache.query_path(pos, {50, 30});
    EXPECT_FALSE(query.is_valid());

    ecs.progress();

    ASSERT_TRUE(query.is_valid());
    EXPECT_EQ(Vector(10, -10), query.get_direction());

    flecs::world movement_ecs;
    set_up_phases(movement_ecs);
    movement_ecs.add<DelaunayPathFindingCache>();
    DelaunayPathFindingCache *movement_cache =
        movement_ecs.try_get_mut<DelaunayPathFindingCache>();
    movement_cache->set_navigator(navigator);
    movement_cache->declare_cache_update_system(movement_ecs, stats);

    EXPECT_EQ(Vector(40, 0),
              get_speed_direction(movement_ecs, pos, {50, 30}, Fixed(100)));
    movement_ecs.progress();
    EXPECT_EQ(Vector(10, -10),
              get_speed_direction(movement_ecs, pos, {50, 30}, Fixed(100)));
}

struct TestGrid
{

	std::size_t nb_tiles_x = 125;
	std::size_t nb_tiles = 125*125;
	octopus::Fixed tile_size = 4;
	uint64_t revision = 0;
    std::vector<bool> free;

    std::size_t get_nb_tiles() const { return nb_tiles; }
    std::size_t get_size_x() const { return nb_tiles_x; }
    octopus::Fixed get_tile_size() const {return tile_size;}
    uint64_t get_revision() const {return revision;}
    bool is_free(std::size_t i) const {return free[i];}
};

TEST(path_finding_cache, basic_query_system)
{
    TimeStats stats;
    flecs::world ecs;
    ecs.add<PathFindingCache>();
    ecs.set<TimeStatsPtr>(TimeStatsPtr {&stats});

    PathFindingCache * cache = ecs.try_get_mut<PathFindingCache>();

    TestGrid grid;
    grid.free.resize(grid.nb_tiles, true);
    for(size_t x = 3 ; x < 12 ; ++x)
    {
        grid.free[10*grid.nb_tiles_x+x] = false;
    }

    for(size_t y = 8 ; y < 10 ; ++y)
    {
        grid.free[y * grid.nb_tiles_x + 3 ] = false;
        grid.free[y * grid.nb_tiles_x + 11] = false;
    }

    set_up_phases(ecs);

    // Position
    Position pos {{30,30}};
    Vector target {50,50};

    cache->declare_sync_system(ecs, &grid);
    cache->declare_cache_update_system(ecs, stats);

    ecs.progress();

    /// query 30,30 -> 50,50 (in coord ths means ~ 7,7 -> 12,12)
    PathQuery query = ecs.try_get<PathFindingCache>()->query_path(pos, target);

    EXPECT_FALSE(query.is_valid());

    ecs.progress();

    ASSERT_TRUE(query.is_valid());

    std::vector<std::size_t> path = cache->build_path(cache->get_index(pos.pos), cache->get_index(target));
    EXPECT_EQ(11, path.size());

    Vector direction_1 = query.get_direction();
    EXPECT_EQ(Vector(4, 0), direction_1);

    pos.pos += direction_1;

    query = cache->query_path(pos, target);

    ASSERT_TRUE(query.is_valid());

    path = cache->build_path(cache->get_index(pos.pos), cache->get_index(target));
    for(size_t p : path)
    {
        std::cout<<cache->get_coord(p)<<std::endl;
    }

    Vector direction_2 = query.get_direction();
    EXPECT_EQ(Vector(4, 0), direction_2);

    pos.pos += direction_2;

    query = cache->query_path(pos, target);

    ASSERT_TRUE(query.is_valid());

    Vector direction_3 = query.get_direction();
    EXPECT_EQ(Vector(4, 0), direction_3);

    std::cout<<"path_finding : "<<stats.path_finding/10e6<<"ms"<<std::endl;
    std::cout<<"path_funnelling : "<<stats.path_funnelling/10e6<<"ms"<<std::endl;
}
