project(flecs LANGUAGES C)

# Automatically pick up files added to src
# file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
#         "${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/*.c")
set(SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/entity.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/entity_name.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/datastructures/sparse.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/datastructures/allocator.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/datastructures/vec.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/datastructures/map.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/datastructures/switch_list.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/datastructures/hashmap.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/datastructures/strbuf.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/datastructures/name_index.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/datastructures/stack_allocator.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/datastructures/hash.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/datastructures/bitset.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/datastructures/block_allocator.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/query/compiler/compiler_term.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/query/compiler/compiler.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/query/api.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/query/util.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/query/engine/eval_member.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/query/engine/eval_up.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/query/engine/eval_union.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/query/engine/eval_trav.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/query/engine/eval.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/query/engine/trav_down_cache.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/query/engine/trav_up_cache.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/query/engine/change_detection.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/query/engine/trav_cache.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/query/engine/eval_pred.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/query/engine/trivial_iter.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/query/engine/cache_order_by.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/query/engine/eval_utils.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/query/engine/eval_toggle.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/query/engine/cache.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/query/engine/eval_iter.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/query/engine/cache_iter.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/query/validator.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/each.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/misc.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/observable.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/iter.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/stage.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/id.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/search.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/addons/pipeline/worker.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/addons/pipeline/pipeline.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/addons/module.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/addons/system/system.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/addons/metrics.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/addons/json/deserialize_value.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/addons/json/deserialize.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/addons/json/serialize_type_info.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/addons/json/serialize_world.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/addons/json/serialize_entity.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/addons/json/serialize_query_info.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/addons/json/serialize_iter_result_table.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/addons/json/json.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/addons/json/serialize_field_info.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/addons/json/serialize_iter_result.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/addons/json/serialize_value.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/addons/json/serialize_iter_result_query.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/addons/json/serialize_iter.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/addons/os_api_impl/os_api_impl.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/addons/log.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/addons/doc.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/addons/stats/pipeline_monitor.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/addons/stats/world_summary.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/addons/stats/system_monitor.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/addons/stats/world_monitor.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/addons/stats/stats.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/addons/stats/monitor.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/addons/rest.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/addons/meta/definitions.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/addons/meta/api.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/addons/meta/meta.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/addons/meta/rtt_lifecycle.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/addons/meta/c_utils.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/addons/meta/serialized.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/addons/meta/cursor.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/addons/journal.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/addons/units.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/addons/alerts.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/addons/timer.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/addons/http.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/addons/app.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/addons/flecs_cpp.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/addons/script/vars.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/addons/script/tokenizer.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/addons/script/query_parser.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/addons/script/visit.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/addons/script/script.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/addons/script/serialize.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/addons/script/interpolate.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/addons/script/visit_eval.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/addons/script/parser.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/addons/script/visit_to_str.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/addons/script/visit_free.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/addons/script/ast.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/addons/script/template.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/addons/script/expr.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/value.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/poly.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/observer.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/os_api.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/storage/table_graph.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/storage/entity_index.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/storage/table_cache.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/storage/table.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/storage/id_index.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/world.c
	${CMAKE_CURRENT_SOURCE_DIR}/flecs/src/bootstrap.c
)

add_library(flecs)
target_sources(flecs PRIVATE ${SOURCES})
target_include_directories(flecs PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include/flecs>
)

# ============================================================================
# Installation
# ============================================================================

install (TARGETS flecs EXPORT octopus-export)

install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/" # source directory
        DESTINATION "include/flecs/" # target directory
        FILES_MATCHING # install only matched files
        PATTERN "*.h" # select header files
)

set (ConfigPackageLocation "lib/cmake/${PROJECT_NAME}-${PROJECT_VERSION}")
