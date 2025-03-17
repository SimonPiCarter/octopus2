project(flecs LANGUAGES C)

# Automatically pick up files added to src
# file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
#         "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")
set(SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/src/entity.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/entity_name.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/datastructures/sparse.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/datastructures/allocator.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/datastructures/vec.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/datastructures/map.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/datastructures/switch_list.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/datastructures/hashmap.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/datastructures/strbuf.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/datastructures/name_index.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/datastructures/stack_allocator.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/datastructures/hash.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/datastructures/bitset.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/datastructures/block_allocator.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/query/compiler/compiler_term.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/query/compiler/compiler.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/query/api.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/query/util.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/query/engine/eval_member.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/query/engine/eval_up.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/query/engine/eval_union.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/query/engine/eval_trav.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/query/engine/eval.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/query/engine/trav_down_cache.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/query/engine/trav_up_cache.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/query/engine/change_detection.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/query/engine/trav_cache.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/query/engine/eval_pred.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/query/engine/trivial_iter.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/query/engine/cache_order_by.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/query/engine/eval_utils.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/query/engine/eval_toggle.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/query/engine/cache.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/query/engine/eval_iter.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/query/engine/cache_iter.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/query/validator.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/each.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/misc.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/observable.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/iter.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/stage.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/id.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/search.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/addons/pipeline/worker.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/addons/pipeline/pipeline.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/addons/module.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/addons/system/system.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/addons/metrics.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/addons/json/deserialize_value.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/addons/json/deserialize.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/addons/json/serialize_type_info.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/addons/json/serialize_world.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/addons/json/serialize_entity.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/addons/json/serialize_query_info.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/addons/json/serialize_iter_result_table.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/addons/json/json.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/addons/json/serialize_field_info.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/addons/json/serialize_iter_result.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/addons/json/serialize_value.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/addons/json/serialize_iter_result_query.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/addons/json/serialize_iter.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/addons/os_api_impl/os_api_impl.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/addons/log.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/addons/doc.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/addons/stats/pipeline_monitor.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/addons/stats/world_summary.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/addons/stats/system_monitor.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/addons/stats/world_monitor.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/addons/stats/stats.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/addons/stats/monitor.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/addons/rest.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/addons/meta/definitions.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/addons/meta/api.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/addons/meta/meta.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/addons/meta/rtt_lifecycle.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/addons/meta/c_utils.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/addons/meta/serialized.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/addons/meta/cursor.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/addons/journal.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/addons/units.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/addons/alerts.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/addons/timer.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/addons/http.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/addons/app.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/addons/flecs_cpp.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/addons/script/expr/ast.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/addons/script/expr/parser.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/addons/script/expr/stack.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/addons/script/expr/util.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/addons/script/expr/visit_eval.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/addons/script/expr/visit_fold.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/addons/script/expr/visit_free.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/addons/script/expr/visit_to_str.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/addons/script/expr/visit_type.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/addons/script/function.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/addons/script/functions_builtin.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/addons/script/functions_math.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/addons/script/vars.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/addons/script/tokenizer.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/addons/script/query_parser.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/addons/script/visit.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/addons/script/script.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/addons/script/serialize.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/addons/script/visit_check.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/addons/script/visit_eval.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/addons/script/parser.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/addons/script/visit_to_str.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/addons/script/visit_free.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/addons/script/ast.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/addons/script/template.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/value.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/poly.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/observer.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/os_api.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/storage/table_graph.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/storage/entity_index.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/storage/table_cache.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/storage/table.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/storage/id_index.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/world.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/bootstrap.c
)

add_library(flecs)
target_sources(flecs PRIVATE ${SOURCES})
target_include_directories(flecs PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
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
