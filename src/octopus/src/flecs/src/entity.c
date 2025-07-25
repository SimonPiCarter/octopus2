/**
 * @file entity.c
 * @brief Entity API.
 * 
 * This file contains the implementation for the entity API, which includes 
 * creating/deleting entities, adding/removing/setting components, instantiating
 * prefabs, and several other APIs for retrieving entity data.
 * 
 * The file also contains the implementation of the command buffer, which is 
 * located here so it can call functions private to the compilation unit.
 */

#include "private_api.h"

#ifdef FLECS_QUERY_DSL
#include "addons/query_dsl/query_dsl.h"
#endif

static
flecs_component_ptr_t flecs_table_get_component(
    ecs_table_t *table,
    int32_t column_index,
    int32_t row)
{
    ecs_check(column_index < table->column_count, ECS_NOT_A_COMPONENT, NULL);
    ecs_column_t *column = &table->data.columns[column_index];
    return (flecs_component_ptr_t){
        .ti = column->ti,
        .ptr = ECS_ELEM(column->data, column->ti->size, row)
    };
error:
    return (flecs_component_ptr_t){0};
}

flecs_component_ptr_t flecs_get_component_ptr(
    ecs_table_t *table,
    int32_t row,
    ecs_component_record_t *cr)
{
    ecs_assert(table != NULL, ECS_INTERNAL_ERROR, NULL);

    if (!cr) {
        return (flecs_component_ptr_t){0};
    }

    if (cr->flags & (EcsIdIsSparse|EcsIdDontFragment)) {
        ecs_entity_t entity = ecs_table_entities(table)[row];
        return (flecs_component_ptr_t){
            .ti = cr->type_info,
            .ptr = flecs_component_sparse_get(cr, entity)
        };
    }

    const ecs_table_record_t *tr = flecs_component_get_table(cr, table);
    if (!tr || (tr->column == -1)) {
        return (flecs_component_ptr_t){0};
    }

    return flecs_table_get_component(table, tr->column, row);
}

void* flecs_get_component(
    ecs_table_t *table,
    int32_t row,
    ecs_component_record_t *cr)
{
    return flecs_get_component_ptr(table, row, cr).ptr;
}

void* flecs_get_base_component(
    const ecs_world_t *world,
    ecs_table_t *table,
    ecs_id_t id,
    ecs_component_record_t *cr,
    int32_t recur_depth)
{
    ecs_check(recur_depth < ECS_MAX_RECURSION, ECS_INVALID_PARAMETER,
        "cycle detected in IsA relationship");

    /* Table (and thus entity) does not have component, look for base */
    if (!(table->flags & EcsTableHasIsA)) {
        return NULL;
    }

    if (!(cr->flags & EcsIdOnInstantiateInherit)) {
        return NULL;
    }

    /* Exclude Name */
    if (id == ecs_pair(ecs_id(EcsIdentifier), EcsName)) {
        return NULL;
    }

    /* Table should always be in the table index for (IsA, *), otherwise the
     * HasBase flag should not have been set */
    const ecs_table_record_t *tr_isa = flecs_component_get_table(
        world->cr_isa_wildcard, table);
    ecs_check(tr_isa != NULL, ECS_INTERNAL_ERROR, NULL);

    ecs_type_t type = table->type;
    ecs_id_t *ids = type.array;
    int32_t i = tr_isa->index, end = tr_isa->count + tr_isa->index;
    void *ptr = NULL;

    do {
        ecs_id_t pair = ids[i ++];
        ecs_entity_t base = ecs_pair_second(world, pair);

        ecs_record_t *r = flecs_entities_get(world, base);
        ecs_assert(r != NULL, ECS_INTERNAL_ERROR, NULL);

        table = r->table;
        ecs_assert(table != NULL, ECS_INTERNAL_ERROR, NULL);

        const ecs_table_record_t *tr = flecs_component_get_table(cr, table);
        if (!tr) {
            if (cr->flags & EcsIdDontFragment) {
                ptr = flecs_component_sparse_get(cr, base);
            }

            if (!ptr) {
                ptr = flecs_get_base_component(world, table, id, cr, 
                    recur_depth + 1);
            }
        } else {
            if (cr->flags & EcsIdIsSparse) {
                return flecs_component_sparse_get(cr, base);
            } else {
                int32_t row = ECS_RECORD_TO_ROW(r->row);
                return flecs_table_get_component(table, tr->column, row).ptr;
            }
        }
    } while (!ptr && (i < end));

    return ptr;
error:
    return NULL;
}

ecs_entity_t flecs_new_id(
    const ecs_world_t *world)
{
    flecs_poly_assert(world, ecs_world_t);

    flecs_check_exclusive_world_access_write(world);

    /* It is possible that the world passed to this function is a stage, so
     * make sure we have the actual world. Cast away const since this is one of
     * the few functions that may modify the world while it is in readonly mode,
     * since it is thread safe (uses atomic inc when in threading mode) */
    ecs_world_t *unsafe_world = ECS_CONST_CAST(ecs_world_t*, world);

    ecs_assert(!(unsafe_world->flags & EcsWorldMultiThreaded),
        ECS_INVALID_OPERATION, "cannot create entities in multithreaded mode");

    ecs_entity_t entity = flecs_entities_new_id(unsafe_world);

    ecs_assert(!unsafe_world->info.max_id || 
        ecs_entity_t_lo(entity) <= unsafe_world->info.max_id, 
        ECS_OUT_OF_RANGE, NULL);

    return entity;
}

static
ecs_record_t* flecs_new_entity(
    ecs_world_t *world,
    ecs_entity_t entity,
    ecs_record_t *record,
    ecs_table_t *table,
    ecs_table_diff_t *diff,
    bool ctor,
    ecs_flags32_t evt_flags)
{
    ecs_assert(record != NULL, ECS_INTERNAL_ERROR, NULL);
    int32_t row = flecs_table_append(world, table, entity, ctor, true);
    record->table = table;
    record->row = ECS_ROW_TO_RECORD(row, record->row & ECS_ROW_FLAGS_MASK);

    ecs_assert(ecs_table_count(table) > row, ECS_INTERNAL_ERROR, NULL);
    flecs_notify_on_add(
        world, table, NULL, row, 1, diff, evt_flags, 0, ctor, true);
    ecs_assert(table == record->table, ECS_INTERNAL_ERROR, NULL);

    return record;
}

static
void flecs_move_entity(
    ecs_world_t *world,
    ecs_entity_t entity,
    ecs_record_t *record,
    ecs_table_t *dst_table,
    ecs_table_diff_t *diff,
    bool ctor,
    ecs_flags32_t evt_flags)
{
    ecs_table_t *src_table = record->table;
    int32_t src_row = ECS_RECORD_TO_ROW(record->row);

    ecs_assert(src_table != NULL, ECS_INTERNAL_ERROR, NULL);
    ecs_assert(src_table != dst_table, ECS_INTERNAL_ERROR, NULL);
    ecs_assert(src_table->type.count >= 0, ECS_INTERNAL_ERROR, NULL);
    ecs_assert(src_row >= 0, ECS_INTERNAL_ERROR, NULL);
    ecs_assert(ecs_table_count(src_table) > src_row, ECS_INTERNAL_ERROR, NULL);
    ecs_check(ecs_is_alive(world, entity), ECS_INVALID_PARAMETER, NULL);
    ecs_assert(record != NULL, ECS_INTERNAL_ERROR, NULL);
    ecs_assert(record == flecs_entities_get(world, entity), 
        ECS_INTERNAL_ERROR, NULL);
    ecs_assert(record->table == src_table, ECS_INTERNAL_ERROR, NULL);

    /* Append new row to destination table */
    int32_t dst_row = flecs_table_append(world, dst_table, entity, 
        false, false);

    /* Invoke remove actions for removed components */
    flecs_notify_on_remove(world, src_table, dst_table, src_row, 1, diff);

    /* Copy entity & components from src_table to dst_table */
    flecs_table_move(world, entity, entity, dst_table, dst_row, 
        src_table, src_row, ctor);
    ecs_assert(record->table == src_table, ECS_INTERNAL_ERROR, NULL);

    /* Update entity index & delete old data after running remove actions */
    record->table = dst_table;
    record->row = ECS_ROW_TO_RECORD(dst_row, record->row & ECS_ROW_FLAGS_MASK);
    
    flecs_table_delete(world, src_table, src_row, false);

    flecs_notify_on_add(world, dst_table, src_table, dst_row, 1, diff, 
        evt_flags, 0, ctor, true);

    ecs_assert(record->table == dst_table, ECS_INTERNAL_ERROR, NULL);
error:
    return;
}

void flecs_commit(
    ecs_world_t *world,
    ecs_entity_t entity,
    ecs_record_t *record,
    ecs_table_t *dst_table,   
    ecs_table_diff_t *diff,
    bool construct,
    ecs_flags32_t evt_flags)
{
    ecs_assert(!(world->flags & EcsWorldReadonly), ECS_INTERNAL_ERROR, NULL);
    flecs_journal_begin(world, EcsJournalMove, entity, 
        &diff->added, &diff->removed);

    ecs_assert(record != NULL, ECS_INTERNAL_ERROR, NULL);
    
    ecs_table_t *src_table = record->table;
    int is_trav = (record->row & EcsEntityIsTraversable) != 0;
    ecs_assert(src_table != NULL, ECS_INTERNAL_ERROR, NULL);

    if (src_table == dst_table) {
        /* If source and destination table are the same no action is needed *
         * However, if a component was added in the process of traversing a
         * table, this suggests that a union relationship could have changed. */
        ecs_flags32_t non_fragment_flags = 
            src_table->flags & EcsTableHasDontFragment;
        if (non_fragment_flags) {
            diff->added_flags |= non_fragment_flags;
            diff->removed_flags |= non_fragment_flags;

            flecs_notify_on_add(world, src_table, src_table, 
                ECS_RECORD_TO_ROW(record->row), 1, diff, evt_flags, 0, 
                    construct, true);

            flecs_notify_on_remove(world, src_table, src_table, 
                ECS_RECORD_TO_ROW(record->row), 1, diff);
        }
        flecs_journal_end();
        return;
    }

    ecs_os_perf_trace_push("flecs.commit");

    ecs_assert(dst_table != NULL, ECS_INTERNAL_ERROR, NULL);
    flecs_table_traversable_add(dst_table, is_trav);

    flecs_move_entity(world, entity, record, dst_table, diff, 
        construct, evt_flags);

    flecs_table_traversable_add(src_table, -is_trav);

    /* If the entity is being watched, it is being monitored for changes and
     * requires rematching systems when components are added or removed. This
     * ensures that systems that rely on components from containers or prefabs
     * update the matched tables when the application adds or removes a 
     * component from, for example, a container. */
    if (is_trav) {
        flecs_update_component_monitors(world, &diff->added, &diff->removed);
    }

    if (!src_table->type.count && world->range_check_enabled) {
        ecs_check(!world->info.max_id || entity <= world->info.max_id, 
            ECS_OUT_OF_RANGE, 0);
        ecs_check(entity >= world->info.min_id, 
            ECS_OUT_OF_RANGE, 0);
    }

    ecs_os_perf_trace_pop("flecs.commit");

error:
    flecs_journal_end();
    return;
}

const ecs_entity_t* flecs_bulk_new(
    ecs_world_t *world,
    ecs_table_t *table,
    const ecs_entity_t *entities,
    ecs_type_t *component_ids,
    int32_t count,
    void **component_data,
    bool is_move,
    int32_t *row_out,
    ecs_table_diff_t *diff)
{
    int32_t sparse_count = 0;
    if (!entities) {
        sparse_count = flecs_entities_count(world);
        entities = flecs_entities_new_ids(world, count);
    }

    ecs_assert(table != NULL, ECS_INTERNAL_ERROR, NULL);

    int32_t row = flecs_table_appendn(world, table, count, entities);

    /* Update entity index. */
    int i;
    for (i = 0; i < count; i ++) {
        ecs_record_t *r = flecs_entities_get(world, entities[i]);
        r->table = table;
        r->row = ECS_ROW_TO_RECORD(row + i, 0);
    }

    ecs_type_t type = table->type;
    if (!type.count && !component_data) {
        return entities;        
    }

    ecs_type_t component_array = { 0 };
    if (!component_ids) {
        component_ids = &component_array;
        component_array.array = type.array;
        component_array.count = type.count;
    }

    flecs_defer_begin(world, world->stages[0]);

    flecs_notify_on_add(world, table, NULL, row, count, diff,
        (component_data == NULL) ? 0 : EcsEventNoOnSet, 0, true, true);

    if (component_data) {
        int32_t c_i;
        for (c_i = 0; c_i < component_ids->count; c_i ++) {
            void *src_ptr = component_data[c_i];
            if (!src_ptr) {
                continue;
            }

            /* Find component in storage type */
            ecs_entity_t id = component_ids->array[c_i];
            ecs_component_record_t *cr = flecs_components_get(world, id);
            ecs_assert(cr != NULL, ECS_INTERNAL_ERROR, NULL);
            const ecs_type_info_t *ti = cr->type_info;
            if (!ti) {
                ecs_assert(ti != NULL, ECS_INVALID_PARAMETER, 
                    "id is not a component");
            }

            int32_t size = ti->size;
            void *ptr;

            if (cr->flags & EcsIdIsSparse) {
                int32_t e;
                for (e = 0; e < count; e ++) {
                    ptr = flecs_component_sparse_get(cr, entities[e]);

                    ecs_copy_t copy;
                    ecs_move_t move;
                    if (is_move && (move = ti->hooks.move)) {
                        move(ptr, src_ptr, 1, ti);
                    } else if (!is_move && (copy = ti->hooks.copy)) {
                        copy(ptr, src_ptr, 1, ti);
                    } else {
                        ecs_os_memcpy(ptr, src_ptr, size);
                    }

                    flecs_notify_on_set(world, table, row + e, id, true);

                    src_ptr = ECS_OFFSET(src_ptr, size);
                }

            } else {
                const ecs_table_record_t *tr = 
                    flecs_component_get_table(cr, table);
                ecs_assert(tr != NULL, ECS_INTERNAL_ERROR, NULL);
                ecs_assert(tr->column != -1, ECS_INVALID_PARAMETER, 
                    "id is not a component");
                ecs_assert(tr->count == 1, ECS_INVALID_PARAMETER,
                    "ids cannot be wildcards");

                int32_t index = tr->column;
                ecs_column_t *column = &table->data.columns[index];
                ecs_assert(size != 0, ECS_INTERNAL_ERROR, NULL);
                ptr = ECS_ELEM(column->data, size, row);

                ecs_copy_t copy;
                ecs_move_t move;
                if (is_move && (move = ti->hooks.move)) {
                    move(ptr, src_ptr, count, ti);
                } else if (!is_move && (copy = ti->hooks.copy)) {
                    copy(ptr, src_ptr, count, ti);
                } else {
                    ecs_os_memcpy(ptr, src_ptr, size * count);
                }
            }
        };

        int32_t j, storage_count = table->column_count;
        for (j = 0; j < storage_count; j ++) {
            ecs_id_t id = flecs_column_id(table, j);
            ecs_type_t set_type = {
                .array = &id,
                .count = 1
            };

            flecs_notify_on_set_ids(world, table, row, count, &set_type);
        }
    }

    flecs_defer_end(world, world->stages[0]);

    if (row_out) {
        *row_out = row;
    }

    if (sparse_count) {
        entities = flecs_entities_ids(world);
        return &entities[sparse_count];
    } else {
        return entities;
    }
}

static
void flecs_add_id_w_record(
    ecs_world_t *world,
    ecs_entity_t entity,
    ecs_record_t *record,
    ecs_id_t id,
    bool construct)
{
    ecs_assert(record != NULL, ECS_INTERNAL_ERROR, NULL);

    ecs_table_t *src_table = record->table;
    ecs_table_diff_t diff = ECS_TABLE_DIFF_INIT;
    ecs_table_t *dst_table = flecs_table_traverse_add(
        world, src_table, &id, &diff);
    flecs_commit(world, entity, record, dst_table, &diff, construct, 
        EcsEventNoOnSet); /* No OnSet, this function is only called from
                           * functions that are about to set the component. */
}

void flecs_add_id(
    ecs_world_t *world,
    ecs_entity_t entity,
    ecs_id_t id)
{
    ecs_stage_t *stage = flecs_stage_from_world(&world);
    if (flecs_defer_add(stage, entity, id)) {
        return;
    }

    ecs_record_t *r = flecs_entities_get(world, entity);
    ecs_assert(r != NULL, ECS_INTERNAL_ERROR, NULL);
    ecs_table_diff_t diff = ECS_TABLE_DIFF_INIT;
    ecs_table_t *src_table = r->table;
    ecs_table_t *dst_table = flecs_table_traverse_add(
        world, src_table, &id, &diff);

    flecs_commit(world, entity, r, dst_table, &diff, true, 0);

    flecs_defer_end(world, stage);
}

void flecs_remove_id(
    ecs_world_t *world,
    ecs_entity_t entity,
    ecs_id_t id)
{
    ecs_stage_t *stage = flecs_stage_from_world(&world);
    if (flecs_defer_remove(stage, entity, id)) {
        return;
    }

    ecs_record_t *r = flecs_entities_get(world, entity);
    ecs_assert(r != NULL, ECS_INTERNAL_ERROR, NULL);
    ecs_table_t *src_table = r->table;
    ecs_table_diff_t diff = ECS_TABLE_DIFF_INIT;
    ecs_table_t *dst_table = flecs_table_traverse_remove(
        world, src_table, &id, &diff);

    flecs_commit(world, entity, r, dst_table, &diff, true, 0);

    flecs_defer_end(world, stage);
}

void flecs_add_ids(
    ecs_world_t *world,
    ecs_entity_t entity,
    ecs_id_t *ids,
    int32_t count)
{
    ecs_record_t *r = flecs_entities_get(world, entity);
    ecs_table_diff_builder_t diff = ECS_TABLE_DIFF_INIT;
    flecs_table_diff_builder_init(world, &diff);

    ecs_table_t *table = ecs_get_table(world, entity);
    int32_t i;
    for (i = 0; i < count; i ++) {
        ecs_id_t id = ids[i];
        table = flecs_find_table_add(world, table, id, &diff);
    }

    ecs_table_diff_t table_diff;
    flecs_table_diff_build_noalloc(&diff, &table_diff);
    flecs_commit(world, entity, r, table, &table_diff, true, 0);
    flecs_table_diff_builder_fini(world, &diff);
}

static
flecs_component_ptr_t flecs_ensure(
    ecs_world_t *world,
    ecs_entity_t entity,
    ecs_entity_t id,
    ecs_record_t *r)
{
    flecs_component_ptr_t dst = {0};

    flecs_poly_assert(world, ecs_world_t);
    ecs_check(id != 0, ECS_INVALID_PARAMETER, NULL);
    ecs_check(r != NULL, ECS_INVALID_PARAMETER, NULL);
    ecs_check((id & ECS_COMPONENT_MASK) == id || 
        ECS_HAS_ID_FLAG(id, PAIR), ECS_INVALID_PARAMETER,
            "invalid component id specified for ensure");

    ecs_component_record_t *cr = NULL;
    ecs_table_t *table = r->table;
    ecs_assert(table != NULL, ECS_INTERNAL_ERROR, NULL);

    if (id < FLECS_HI_COMPONENT_ID) {
        int16_t column_index = table->component_map[id];
        if (column_index > 0) {
            ecs_column_t *column = &table->data.columns[column_index - 1];
            ecs_type_info_t *ti = column->ti;
            dst.ptr = ECS_ELEM(column->data, ti->size, 
                ECS_RECORD_TO_ROW(r->row));
            dst.ti = ti;
            return dst;
        } else if (column_index < 0) {
            column_index = flecs_ito(int16_t, -column_index - 1);
            const ecs_table_record_t *tr = &table->_->records[column_index];
            cr = (ecs_component_record_t*)tr->hdr.cache;
            if (cr->flags & EcsIdIsSparse) {
                dst.ptr = flecs_component_sparse_get(cr, entity);
                dst.ti = cr->type_info;
                return dst;
            }
        }
    } else {
        cr = flecs_components_get(world, id);
        dst = flecs_get_component_ptr(table, ECS_RECORD_TO_ROW(r->row), cr);
        if (dst.ptr) {
            return dst;
        }
    }

    /* If entity didn't have component yet, add it */
    flecs_add_id_w_record(world, entity, r, id, true);

    /* Flush commands so the pointer we're fetching is stable */
    flecs_defer_end(world, world->stages[0]);
    flecs_defer_begin(world, world->stages[0]);

    if (!cr) {
        cr = flecs_components_get(world, id);
        ecs_assert(cr != NULL, ECS_INTERNAL_ERROR, NULL);
    }

    ecs_assert(r->table != NULL, ECS_INTERNAL_ERROR, NULL);
    dst = flecs_get_component_ptr(r->table, ECS_RECORD_TO_ROW(r->row), cr);
error:
    return dst;
}

void flecs_record_add_flag(
    ecs_record_t *record,
    uint32_t flag)
{
    if (flag == EcsEntityIsTraversable) {
        if (!(record->row & flag)) {
            ecs_table_t *table = record->table;
            ecs_assert(table != NULL, ECS_INTERNAL_ERROR, NULL);
            flecs_table_traversable_add(table, 1);
        }
    }
    record->row |= flag;
}

void flecs_add_flag(
    ecs_world_t *world,
    ecs_entity_t entity,
    uint32_t flag)
{
    ecs_record_t *record = flecs_entities_get_any(world, entity);
    ecs_assert(record != NULL, ECS_INTERNAL_ERROR, NULL);
    flecs_record_add_flag(record, flag);
}

void flecs_add_to_root_table(
    ecs_world_t *world,
    ecs_stage_t *stage,
    ecs_entity_t e)
{
    flecs_poly_assert(world, ecs_world_t);

    if (world->flags & EcsWorldMultiThreaded) {
        if (flecs_defer_new(stage, e)) {
            return;
        }
        ecs_abort(ECS_INTERNAL_ERROR, NULL); /* Can't happen */
    }

    ecs_record_t *r = flecs_entities_get(world, e);
    ecs_assert(r != NULL, ECS_INTERNAL_ERROR, NULL);
    ecs_assert(r->table == NULL, ECS_INTERNAL_ERROR, NULL);

    ecs_table_diff_t diff = ECS_TABLE_DIFF_INIT;
    flecs_new_entity(world, e, r, &world->store.root, &diff, false, 0);
    ecs_assert(r->table == &world->store.root, ECS_INTERNAL_ERROR, NULL);

    flecs_journal(world, EcsJournalNew, e, 0, 0);
}


/* -- Public functions -- */

bool ecs_commit(
    ecs_world_t *world,
    ecs_entity_t entity,
    ecs_record_t *record,
    ecs_table_t *table,
    const ecs_type_t *added,
    const ecs_type_t *removed)
{
    ecs_check(world != NULL, ECS_INVALID_PARAMETER, NULL);
    ecs_check(!ecs_is_deferred(world), ECS_INVALID_OPERATION, 
        "commit cannot be called on stage or while world is deferred");

    ecs_table_t *src_table = NULL;
    if (!record) {
        record = flecs_entities_get(world, entity);
        src_table = record->table;
    }

    ecs_table_diff_t diff = ECS_TABLE_DIFF_INIT;

    if (added) {
        diff.added = *added;
        diff.added_flags = table->flags & EcsTableAddEdgeFlags;
    }
    if (removed) {
        diff.removed = *removed;
        if (src_table) {
            diff.removed_flags = src_table->flags & EcsTableRemoveEdgeFlags;
        }
    }

    ecs_defer_begin(world);
    flecs_commit(world, entity, record, table, &diff, true, 0);
    ecs_defer_end(world);

    return src_table != table;
error:
    return false;
}

ecs_entity_t ecs_new(
    ecs_world_t *world)
{
    ecs_stage_t *stage = flecs_stage_from_world(&world);
    ecs_entity_t e = flecs_new_id(world);
    flecs_add_to_root_table(world, stage, e);
    return e;
}

ecs_entity_t ecs_new_low_id(
    ecs_world_t *world)
{
    ecs_check(world != NULL, ECS_INVALID_PARAMETER, NULL);

    ecs_stage_t *stage = flecs_stage_from_world(&world);

    flecs_check_exclusive_world_access_write(world);

    if (world->flags & EcsWorldReadonly) {
        /* Can't issue new comp id while iterating when in multithreaded mode */
        ecs_check(ecs_get_stage_count(world) <= 1,
            ECS_INVALID_WHILE_READONLY, NULL);
    }

    ecs_entity_t e = 0;
    if (world->info.last_component_id < FLECS_HI_COMPONENT_ID) {
        do {
            e = world->info.last_component_id ++;
        } while (ecs_exists(world, e) && e <= FLECS_HI_COMPONENT_ID);        
    }

    if (!e || e >= FLECS_HI_COMPONENT_ID) {
        /* If the low component ids are depleted, return a regular entity id */
        e = ecs_new(world);
    } else {
        flecs_entities_ensure(world, e);
        flecs_add_to_root_table(world, stage, e);
    }

    return e;
error: 
    return 0;
}

ecs_entity_t ecs_new_w_table(
    ecs_world_t *world,
    ecs_table_t *table)
{
    ecs_check(world != NULL, ECS_INVALID_PARAMETER, NULL);
    ecs_check(table != NULL, ECS_INVALID_PARAMETER, NULL);

    flecs_stage_from_world(&world);    
    ecs_entity_t entity = flecs_new_id(world);
    ecs_record_t *r = flecs_entities_get(world, entity);
    ecs_flags32_t flags = table->flags & EcsTableAddEdgeFlags;
    if (table->flags & EcsTableHasIsA) {
        flags |= EcsTableHasOnAdd;
    }

    ecs_table_diff_t table_diff = { 
        .added = table->type,
        .added_flags = flags
    };

    flecs_new_entity(world, entity, r, table, &table_diff, true, 0);

    return entity;
error:
    return 0;
}

ecs_entity_t ecs_new_w_id(
    ecs_world_t *world,
    ecs_id_t id)
{
    ecs_check(world != NULL, ECS_INVALID_PARAMETER, NULL);
    ecs_check(ecs_id_is_valid(world, id), ECS_INVALID_PARAMETER, NULL);

    ecs_stage_t *stage = flecs_stage_from_world(&world);

    if (flecs_defer_cmd(stage)) {
        ecs_assert(ecs_is_deferred(world), ECS_INTERNAL_ERROR, NULL);
        ecs_entity_t e = ecs_new(world);
        ecs_add_id(world, e, id);
        return e;
    }

    ecs_table_diff_t table_diff = ECS_TABLE_DIFF_INIT;
    ecs_table_t *table = flecs_table_traverse_add(
        world, &world->store.root, &id, &table_diff);
    
    ecs_entity_t entity = flecs_new_id(world);
    ecs_record_t *r = flecs_entities_get(world, entity);
    flecs_new_entity(world, entity, r, table, &table_diff, true, 0);

    flecs_defer_end(world, stage);

    ecs_assert(!ecs_is_deferred(world), ECS_INTERNAL_ERROR, NULL);

    return entity;
error:
    return 0;
}

static
void flecs_copy_id(
    ecs_world_t *world,
    ecs_record_t *r,
    ecs_id_t id,
    size_t size,
    void *dst_ptr,
    void *src_ptr,
    const ecs_type_info_t *ti)
{
    ecs_check(dst_ptr != NULL, ECS_INVALID_PARAMETER, NULL);
    ecs_check(src_ptr != NULL, ECS_INVALID_PARAMETER, NULL);

    ecs_copy_t copy = ti->hooks.copy;
    if (copy) {
        copy(dst_ptr, src_ptr, 1, ti);
    } else {
        ecs_os_memcpy(dst_ptr, src_ptr, flecs_utosize(size));
    }

    flecs_table_mark_dirty(world, r->table, id);

    ecs_table_t *table = r->table;
    ecs_assert(table != NULL, ECS_INTERNAL_ERROR, NULL);

    flecs_notify_on_set(
        world, table, ECS_RECORD_TO_ROW(r->row), id, true);
error:
    return;
}

/* Traverse table graph by either adding or removing identifiers parsed from the
 * passed in expression. */
static
int flecs_traverse_from_expr(
    ecs_world_t *world,
    const char *name,
    const char *expr,
    ecs_vec_t *ids)
{
#ifdef FLECS_QUERY_DSL
    const char *ptr = expr;
    if (ptr) {
        ecs_id_t id = 0;
        while (ptr[0] && (ptr = flecs_id_parse(world, name, ptr, &id))) {
            if (!id) {
                break;
            }

            if (!ecs_id_is_valid(world, id)) {
                char *idstr = ecs_id_str(world, id);
                ecs_parser_error(name, expr, (ptr - expr), 
                    "id %s is invalid for add expression", idstr);
                ecs_os_free(idstr);
                goto error;
            }

            ecs_vec_append_t(&world->allocator, ids, ecs_id_t)[0] = id;
        }

        if (!ptr) {
            goto error;
        }
    }
    return 0;
#else
    (void)world;
    (void)name;
    (void)expr;
    (void)ids;
    ecs_err("cannot parse component expression: script addon required");
    goto error;
#endif
error:
    return -1;
}

/* Add/remove components based on the parsed expression. This operation is 
 * slower than flecs_traverse_from_expr, but safe to use from a deferred context. */
static
void flecs_defer_from_expr(
    ecs_world_t *world,
    ecs_entity_t entity,
    const char *name,
    const char *expr)
{
#ifdef FLECS_QUERY_DSL
    const char *ptr = expr;
    if (ptr) {
        ecs_id_t id = 0;
        while (ptr[0] && (ptr = flecs_id_parse(world, name, ptr, &id))) {
            if (!id) {
                break;
            }
            ecs_add_id(world, entity, id);
        }
    }
#else
    (void)world;
    (void)entity;
    (void)name;
    (void)expr;
    ecs_err("cannot parse component expression: script addon required");
#endif
}

/* If operation is not deferred, add components by finding the target
 * table and moving the entity towards it. */
static 
int flecs_traverse_add(
    ecs_world_t *world,
    ecs_entity_t result,
    const char *name,
    const ecs_entity_desc_t *desc,
    ecs_entity_t scope,
    ecs_id_t with,
    bool new_entity,
    bool name_assigned)
{
    const char *sep = desc->sep;
    const char *root_sep = desc->root_sep;
    ecs_table_diff_builder_t diff = ECS_TABLE_DIFF_INIT;
    flecs_table_diff_builder_init(world, &diff);
    ecs_vec_t ids;

    /* Add components from the 'add_expr' expression. Look up before naming 
     * entity, so that expression can't resolve to self. */
    ecs_vec_init_t(&world->allocator, &ids, ecs_id_t, 0);
    if (desc->add_expr && ecs_os_strcmp(desc->add_expr, "0")) {
        if (flecs_traverse_from_expr(world, name, desc->add_expr, &ids)) {
            goto error;
        }
    }

    /* Set symbol */
    if (desc->symbol && desc->symbol[0]) {
        const char *sym = ecs_get_symbol(world, result);
        if (sym) {
            ecs_assert(!ecs_os_strcmp(desc->symbol, sym),
                ECS_INCONSISTENT_NAME, "%s (provided) vs. %s (existing)",
                    desc->symbol, sym);
        } else {
            ecs_set_symbol(world, result, desc->symbol);
        }
    }

    /* If a name is provided but not yet assigned, add the Name component */
    if (name && !name_assigned) {
        if (!ecs_add_path_w_sep(world, result, scope, name, sep, root_sep)) {
            if (name[0] == '#') {
                /* Numerical ids should always return, unless it's invalid */
                goto error;
            }
        }
    } else if (new_entity && scope) {
        ecs_add_pair(world, result, EcsChildOf, scope);
    }

    /* Find existing table */
    ecs_table_t *src_table = NULL, *table = NULL;
    ecs_record_t *r = flecs_entities_get(world, result);
    table = r->table;

    /* Add components from the 'add' array */
    if (desc->add) {
        int32_t i = 0;
        ecs_id_t id;

        while ((id = desc->add[i ++])) {
            table = flecs_find_table_add(world, table, id, &diff);
        }
    }

    /* Add components from the 'set' array */
    if (desc->set) {
        int32_t i = 0;
        ecs_id_t id;

        while ((id = desc->set[i ++].type)) {
            table = flecs_find_table_add(world, table, id, &diff);
        }
    }

    /* Add ids from .expr */
    {
        int32_t i, count = ecs_vec_count(&ids);
        ecs_id_t *expr_ids = ecs_vec_first(&ids);
        for (i = 0; i < count; i ++) {
            table = flecs_find_table_add(world, table, expr_ids[i], &diff);
        }
    }

    /* Find destination table */
    /* If this is a new entity without a name, add the scope. If a name is
     * provided, the scope will be added by the add_path_w_sep function */
    if (new_entity) {
        if (new_entity && scope && !name && !name_assigned) {
            table = flecs_find_table_add(
                world, table, ecs_pair(EcsChildOf, scope), &diff);
        }
        if (with) {
            table = flecs_find_table_add(world, table, with, &diff);
        }
    }

    /* Commit entity to destination table */
    if (src_table != table) {
        flecs_defer_begin(world, world->stages[0]);
        ecs_table_diff_t table_diff;
        flecs_table_diff_build_noalloc(&diff, &table_diff);
        flecs_commit(world, result, r, table, &table_diff, true, 0);
        flecs_table_diff_builder_fini(world, &diff);
        flecs_defer_end(world, world->stages[0]);
    }

    /* Set component values */
    if (desc->set) {
        table = r->table;
        ecs_assert(r->table != NULL, ECS_INTERNAL_ERROR, NULL);
        int32_t i = 0, row = ECS_RECORD_TO_ROW(r->row);
        const ecs_value_t *v;
        
        flecs_defer_begin(world, world->stages[0]);

        while ((void)(v = &desc->set[i ++]), v->type) {
            if (!v->ptr) {
                continue;
            }
            ecs_assert(ECS_RECORD_TO_ROW(r->row) == row, ECS_INTERNAL_ERROR, NULL);
            ecs_component_record_t *cr = flecs_components_get(world, v->type);
            flecs_component_ptr_t ptr = flecs_get_component_ptr(table, row, cr);
            ecs_check(ptr.ptr != NULL, ECS_INTERNAL_ERROR, NULL);
            const ecs_type_info_t *ti = cr->type_info;
            flecs_copy_id(world, r, v->type, 
                flecs_itosize(ti->size), ptr.ptr, v->ptr, ti);
        }

        flecs_defer_end(world, world->stages[0]);
    }

    flecs_table_diff_builder_fini(world, &diff);
    ecs_vec_fini_t(&world->allocator, &ids, ecs_id_t);
    return 0;
error:
    flecs_table_diff_builder_fini(world, &diff);
    ecs_vec_fini_t(&world->allocator, &ids, ecs_id_t);
    return -1;
}

/* When in deferred mode, we need to add/remove components one by one using
 * the regular operations. */
static 
void flecs_deferred_add_remove(
    ecs_world_t *world,
    ecs_entity_t entity,
    const char *name,
    const ecs_entity_desc_t *desc,
    ecs_entity_t scope,
    ecs_id_t with,
    bool flecs_new_entity,
    bool name_assigned)
{
    const char *sep = desc->sep;
    const char *root_sep = desc->root_sep;

    /* If this is a new entity without a name, add the scope. If a name is
     * provided, the scope will be added by the add_path_w_sep function */
    if (flecs_new_entity) {
        if (flecs_new_entity && scope && !name && !name_assigned) {
            ecs_add_id(world, entity, ecs_pair(EcsChildOf, scope));
        }

        if (with) {
            ecs_add_id(world, entity, with);
        }
    }

    /* Add components from the 'add' id array */
    if (desc->add) {
        int32_t i = 0;
        ecs_id_t id;

        while ((id = desc->add[i ++])) {
            bool defer = true;
            if (ECS_HAS_ID_FLAG(id, PAIR) && ECS_PAIR_FIRST(id) == EcsChildOf) {
                scope = ECS_PAIR_SECOND(id);
                if (name && (!desc->id || !name_assigned)) {
                    /* New named entities are created by temporarily going out of
                     * readonly mode to ensure no duplicates are created. */
                    defer = false;
                }
            }
            if (defer) {
                ecs_add_id(world, entity, id);
            }
        }
    }

    /* Set component values */
    if (desc->set) {
        int32_t i = 0;
        const ecs_value_t *v;
        while ((void)(v = &desc->set[i ++]), v->type) {
            if (v->ptr) {
                ecs_set_id(world, entity, v->type, 0, v->ptr);
            } else {
                ecs_add_id(world, entity, v->type);
            }
        }
    }

    /* Add components from the 'add_expr' expression */
    if (desc->add_expr) {
        flecs_defer_from_expr(world, entity, name, desc->add_expr);
    }

    int32_t thread_count = ecs_get_stage_count(world);

    /* Set symbol */
    if (desc->symbol) {
        const char *sym = ecs_get_symbol(world, entity);
        if (!sym || ecs_os_strcmp(sym, desc->symbol)) {
            if (thread_count <= 1) { /* See above */
                ecs_suspend_readonly_state_t state;
                ecs_world_t *real_world = flecs_suspend_readonly(world, &state);
                ecs_set_symbol(world, entity, desc->symbol);
                flecs_resume_readonly(real_world, &state);
            } else {
                ecs_set_symbol(world, entity, desc->symbol);
            }
        }
    }

    /* Set name */
    if (name && !name_assigned) {
        ecs_add_path_w_sep(world, entity, scope, name, sep, root_sep);
    }
}

ecs_entity_t ecs_entity_init(
    ecs_world_t *world,
    const ecs_entity_desc_t *desc)
{
    ecs_check(world != NULL, ECS_INVALID_PARAMETER, NULL);
    ecs_check(desc != NULL, ECS_INVALID_PARAMETER, NULL);
    ecs_check(desc->_canary == 0, ECS_INVALID_PARAMETER,
        "ecs_entity_desc_t was not initialized to zero");

    ecs_stage_t *stage = flecs_stage_from_world(&world);
    ecs_entity_t scope = stage->scope;
    ecs_id_t with = ecs_get_with(world);
    ecs_entity_t result = desc->id;

#ifdef FLECS_DEBUG
    if (desc->add) {
        ecs_id_t id;
        int32_t i = 0;
        while ((id = desc->add[i ++])) {
            if (ECS_HAS_ID_FLAG(id, PAIR) && 
                (ECS_PAIR_FIRST(id) == EcsChildOf))
            {
                if (desc->name) {
                    ecs_check(false, ECS_INVALID_PARAMETER, "%s: cannot set parent in "
                        "ecs_entity_desc_t::add, use ecs_entity_desc_t::parent",
                            desc->name);
                } else {
                    ecs_check(false, ECS_INVALID_PARAMETER, "cannot set parent in "
                        "ecs_entity_desc_t::add, use ecs_entity_desc_t::parent");
                }
            }
        }
    }
#endif

    const char *name = desc->name;
    const char *sep = desc->sep;
    if (!sep) {
        sep = ".";
    }

    if (name) {
        if (!name[0]) {
            name = NULL;
        } else if (flecs_name_is_id(name)){
            ecs_entity_t id = flecs_name_to_id(name);
            if (!id) {
                return 0;
            }
            if (result && (id != result)) {
                ecs_err("name id conflicts with provided id");
                return 0;
            }
            name = NULL;
            result = id;
        }
    }

    const char *root_sep = desc->root_sep;
    bool flecs_new_entity = false;
    bool name_assigned = false;

    /* Remove optional prefix from name. Entity names can be derived from 
     * language identifiers, such as components (typenames) and systems
     * function names). Because C does not have namespaces, such identifiers
     * often encode the namespace as a prefix.
     * To ensure interoperability between C and C++ (and potentially other 
     * languages with namespacing) the entity must be stored without this prefix
     * and with the proper namespace, which is what the name_prefix is for */
    const char *prefix = world->info.name_prefix;
    if (name && prefix) {
        ecs_size_t len = ecs_os_strlen(prefix);
        if (!ecs_os_strncmp(name, prefix, len) && 
           (isupper(name[len]) || name[len] == '_')) 
        {
            if (name[len] == '_') {
                name = name + len + 1;
            } else {
                name = name + len;
            }
        }
    }

    /* Parent field takes precedence over scope */
    if (desc->parent) {
        scope = desc->parent;
        ecs_check(ecs_is_valid(world, desc->parent), 
            ECS_INVALID_PARAMETER, "ecs_entity_desc_t::parent is not valid");
    }

    /* Find or create entity */
    if (!result) {
        if (name) {
            /* If add array contains a ChildOf pair, use it as scope instead */
            result = ecs_lookup_path_w_sep(
                world, scope, name, sep, root_sep, false);
            if (result) {
                name_assigned = true;
            }
        }

        if (!result) {
            if (desc->use_low_id) {
                result = ecs_new_low_id(world);
            } else {
                result = ecs_new(world);
            }
            flecs_new_entity = true;
            ecs_assert(ecs_get_type(world, result) != NULL,
                ECS_INTERNAL_ERROR, NULL);
            ecs_assert(ecs_get_type(world, result)->count == 0,
                ECS_INTERNAL_ERROR, NULL);
        }
    } else {
        /* Make sure provided id is either alive or revivable */
        ecs_make_alive(world, result);

        name_assigned = ecs_has_pair(
            world, result, ecs_id(EcsIdentifier), EcsName);
        if (name && name_assigned) {
            /* If entity has name, verify that name matches. The name provided
             * to the function could either have been relative to the current
             * scope, or fully qualified. */
            char *path;
            ecs_size_t root_sep_len = root_sep ? ecs_os_strlen(root_sep) : 0;
            if (root_sep && !ecs_os_strncmp(name, root_sep, root_sep_len)) {
                /* Fully qualified name was provided, so make sure to
                 * compare with fully qualified name */
                path = ecs_get_path_w_sep(world, 0, result, sep, root_sep);
            } else {
                /* Relative name was provided, so make sure to compare with
                 * relative name */
                if (!sep || sep[0]) {
                    path = ecs_get_path_w_sep(world, scope, result, sep, "");
                } else {
                    /* Safe, only freed when sep is valid */
                    path = ECS_CONST_CAST(char*, ecs_get_name(world, result));
                }
            }
            if (path) {
                if (ecs_os_strcmp(path, name)) {
                    /* Mismatching name */
                    ecs_err("existing entity '%s' is initialized with "
                        "conflicting name '%s'", path, name);
                    if (!sep || sep[0]) {
                        ecs_os_free(path);
                    }
                    return 0;
                }
                if (!sep || sep[0]) {
                    ecs_os_free(path);
                }
            }
        }
    }

    ecs_assert(name_assigned == ecs_has_pair(
        world, result, ecs_id(EcsIdentifier), EcsName),
            ECS_INTERNAL_ERROR, NULL);

    if (ecs_is_deferred(world)) {
        flecs_deferred_add_remove((ecs_world_t*)stage, result, name, desc, 
            scope, with, flecs_new_entity, name_assigned);
    } else {
        if (flecs_traverse_add(world, result, name, desc,
            scope, with, flecs_new_entity, name_assigned)) 
        {
            return 0;
        }
    }

    return result;
error:
    return 0;
}

const ecs_entity_t* ecs_bulk_init(
    ecs_world_t *world,
    const ecs_bulk_desc_t *desc)
{
    flecs_poly_assert(world, ecs_world_t);
    ecs_assert(!(world->flags & EcsWorldReadonly), ECS_INTERNAL_ERROR, NULL);
    ecs_check(desc != NULL, ECS_INVALID_PARAMETER, NULL);
    ecs_check(desc->_canary == 0, ECS_INVALID_PARAMETER,
        "ecs_bulk_desc_t was not initialized to zero");

    flecs_check_exclusive_world_access_write(world);

    const ecs_entity_t *entities = desc->entities;
    int32_t count = desc->count;

    int32_t sparse_count = 0;
    if (!entities) {
        sparse_count = flecs_entities_count(world);
        entities = flecs_entities_new_ids(world, count);
        ecs_assert(entities != NULL, ECS_INTERNAL_ERROR, NULL);
    } else {
        int i;
        for (i = 0; i < count; i ++) {
            flecs_entities_ensure(world, entities[i]);
        }
    }

    ecs_type_t ids;
    ecs_table_t *table = desc->table;
    if (!table) {
        table = &world->store.root;
    }

    if (!table->type.count) {
        ecs_table_diff_builder_t diff = ECS_TABLE_DIFF_INIT;
        flecs_table_diff_builder_init(world, &diff);

        int32_t i = 0;
        ecs_id_t id;
        while ((id = desc->ids[i])) {
            table = flecs_find_table_add(world, table, id, &diff);
            i ++;
        }

        ids.array = ECS_CONST_CAST(ecs_id_t*, desc->ids);
        ids.count = i;

        ecs_table_diff_t table_diff;
        flecs_table_diff_build_noalloc(&diff, &table_diff);
        flecs_bulk_new(world, table, entities, &ids, count, desc->data, true, NULL, 
            &table_diff);
        flecs_table_diff_builder_fini(world, &diff);
    } else {
        ecs_table_diff_t diff = {
            .added.array = table->type.array,
            .added.count = table->type.count
        };

        int32_t i = 0;
        while ((desc->ids[i])) {
            i ++;
        }

        ids.array = ECS_CONST_CAST(ecs_id_t*, desc->ids);
        ids.count = i;

        flecs_bulk_new(world, table, entities, &ids, count, desc->data, true, NULL, 
            &diff);
    }

    if (!sparse_count) {
        return entities;
    } else {
        /* Refetch entity ids, in case the underlying array was reallocated */
        entities = flecs_entities_ids(world);
        return &entities[sparse_count];
    }
error:
    return NULL;
}

const ecs_entity_t* ecs_bulk_new_w_id(
    ecs_world_t *world,
    ecs_id_t id,
    int32_t count)
{
    ecs_check(world != NULL, ECS_INVALID_PARAMETER, NULL);
    ecs_stage_t *stage = flecs_stage_from_world(&world);

    const ecs_entity_t *ids;
    if (flecs_defer_bulk_new(world, stage, count, id, &ids)) {
        return ids;
    }

    ecs_table_t *table = &world->store.root;
    ecs_table_diff_builder_t diff = ECS_TABLE_DIFF_INIT;
    flecs_table_diff_builder_init(world, &diff);
    
    if (id) {
        table = flecs_find_table_add(world, table, id, &diff);
    }

    ecs_table_diff_t td;
    flecs_table_diff_build_noalloc(&diff, &td);
    ids = flecs_bulk_new(world, table, NULL, NULL, count, NULL, false, NULL, &td);
    flecs_table_diff_builder_fini(world, &diff);
    flecs_defer_end(world, stage);

    return ids;
error:
    return NULL;
}

static
void flecs_check_component(
    ecs_world_t *world,
    ecs_entity_t result,
    const EcsComponent *ptr,
    ecs_size_t size,
    ecs_size_t alignment)
{
    if (ptr->size != size) {
        char *path = ecs_get_path(world, result);
        ecs_abort(ECS_INVALID_COMPONENT_SIZE, path);
        ecs_os_free(path);
    }
    if (ptr->alignment != alignment) {
        char *path = ecs_get_path(world, result);
        ecs_abort(ECS_INVALID_COMPONENT_ALIGNMENT, path);
        ecs_os_free(path);
    }
}

ecs_entity_t ecs_component_init(
    ecs_world_t *world,
    const ecs_component_desc_t *desc)
{
    ecs_check(world != NULL, ECS_INVALID_PARAMETER, NULL);
    ecs_check(desc != NULL, ECS_INVALID_PARAMETER, NULL);
    ecs_check(desc->_canary == 0, ECS_INVALID_PARAMETER,
        "ecs_component_desc_t was not initialized to 0");

    /* If existing entity is provided, check if it is already registered as a
     * component and matches the size/alignment. This can prevent having to
     * suspend readonly mode, and increases the number of scenarios in which
     * this function can be called in multithreaded mode. */
    ecs_entity_t result = desc->entity;
    if (result && ecs_is_alive(world, result)) {
        const EcsComponent *const_ptr = ecs_get(world, result, EcsComponent);
        if (const_ptr) {
            flecs_check_component(world, result, const_ptr,
                desc->type.size, desc->type.alignment);
            return result;
        }
    }

    ecs_suspend_readonly_state_t readonly_state;
    world = flecs_suspend_readonly(world, &readonly_state);

    bool new_component = true;
    if (!result) {
        result = ecs_new_low_id(world);
    } else {
        ecs_make_alive(world, result);
        new_component = ecs_has(world, result, EcsComponent);
    }

    EcsComponent *ptr = ecs_ensure(world, result, EcsComponent);
    if (!ptr->size) {
        ecs_assert(ptr->alignment == 0, ECS_INTERNAL_ERROR, NULL);
        ptr->size = desc->type.size;
        ptr->alignment = desc->type.alignment;
        if (!new_component || ptr->size != desc->type.size) {
            if (!ptr->size) {
                ecs_trace("#[green]tag#[reset] %s registered", 
                    ecs_get_name(world, result));
            } else {
                ecs_trace("#[green]component#[reset] %s registered", 
                    ecs_get_name(world, result));
            }
        }
    } else {
        flecs_check_component(world, result, ptr,
            desc->type.size, desc->type.alignment);
    }

    if (desc->type.name && new_component) {
        ecs_entity(world, { .id = result, .name = desc->type.name });
    }

    ecs_modified(world, result, EcsComponent);

    if (desc->type.size && 
        !ecs_id_in_use(world, result) && 
        !ecs_id_in_use(world, ecs_pair(result, EcsWildcard)))
    {
        ecs_set_hooks_id(world, result, &desc->type.hooks);
    }

    if (result >= world->info.last_component_id && result < FLECS_HI_COMPONENT_ID) {
        world->info.last_component_id = result + 1;
    }

    flecs_resume_readonly(world, &readonly_state);

    ecs_assert(result != 0, ECS_INTERNAL_ERROR, NULL);
    ecs_assert(ecs_has(world, result, EcsComponent), ECS_INTERNAL_ERROR, NULL);

    return result;
error:
    return 0;
}

void ecs_clear(
    ecs_world_t *world,
    ecs_entity_t entity)
{
    ecs_check(world != NULL, ECS_INVALID_PARAMETER, NULL);
    ecs_check(ecs_is_valid(world, entity), ECS_INVALID_PARAMETER, NULL);

    ecs_stage_t *stage = flecs_stage_from_world(&world);
    if (flecs_defer_clear(stage, entity)) {
        return;
    }

    ecs_record_t *r = flecs_entities_get(world, entity);
    ecs_assert(r != NULL, ECS_INTERNAL_ERROR, NULL);

    ecs_table_t *table = r->table;
    ecs_assert(table != NULL, ECS_INTERNAL_ERROR, NULL);
    if (table->type.count) {
        ecs_table_diff_t diff = {
            .removed = table->type,
            .removed_flags = table->flags & EcsTableRemoveEdgeFlags
        };

        flecs_commit(world, entity, r, &world->store.root, &diff, false, 0);
    }

    flecs_entity_remove_non_fragmenting(world, entity, NULL);

    flecs_defer_end(world, stage);
error:
    return;
}

void ecs_delete(
    ecs_world_t *world,
    ecs_entity_t entity)
{
    ecs_check(world != NULL, ECS_INVALID_PARAMETER, NULL);
    ecs_check(entity != 0, ECS_INVALID_PARAMETER, NULL);

    ecs_stage_t *stage = flecs_stage_from_world(&world);
    if (flecs_defer_delete(stage, entity)) {
        return;
    }

    ecs_os_perf_trace_push("flecs.delete");

    ecs_record_t *r = flecs_entities_try(world, entity);
    if (r) {
        flecs_journal_begin(world, EcsJournalDelete, entity, NULL, NULL);

        ecs_flags32_t row_flags = ECS_RECORD_TO_ROW_FLAGS(r->row);
        ecs_table_t *table;
        if (row_flags) {
            if (row_flags & EcsEntityIsTarget) {
                flecs_on_delete(world, ecs_pair(EcsFlag, entity), 0, true);
                flecs_on_delete(world, ecs_pair(EcsWildcard, entity), 0, true);
                r->cr = NULL;
            }

            if (row_flags & EcsEntityIsId) {
                flecs_on_delete(world, entity, 0, true);
                flecs_on_delete(world, ecs_pair(entity, EcsWildcard), 0, true);
            }

            if (row_flags & EcsEntityIsTraversable) {
                flecs_table_traversable_add(r->table, -1);
            }

            /* Remove non-fragmenting components */
            flecs_entity_remove_non_fragmenting(world, entity, r);

            /* Merge operations before deleting entity */
            flecs_defer_end(world, stage);
            flecs_defer_begin(world, stage);
        }

        table = r->table;

        if (table) { /* NULL if entity got cleaned up as result of cycle */
            ecs_table_diff_t diff = {
                .removed = table->type,
                .removed_flags = table->flags & EcsTableRemoveEdgeFlags
            };

            int32_t row = ECS_RECORD_TO_ROW(r->row);
            flecs_notify_on_remove(
                world, table, &world->store.root, row, 1, &diff);
            flecs_table_delete(world, table, row, true);
        }
        
        flecs_entities_remove(world, entity);

        flecs_journal_end();
    }

    flecs_defer_end(world, stage);
error:
    ecs_os_perf_trace_pop("flecs.delete");
    return;
}

void ecs_add_id(
    ecs_world_t *world,
    ecs_entity_t entity,
    ecs_id_t id)
{
    ecs_check(world != NULL, ECS_INVALID_PARAMETER, NULL);
    ecs_check(ecs_is_alive(world, entity), ECS_INVALID_PARAMETER, NULL);
    ecs_check(ecs_id_is_valid(world, id), ECS_INVALID_PARAMETER, NULL);
    flecs_add_id(world, entity, id);
error:
    return;
}

void ecs_remove_id(
    ecs_world_t *world,
    ecs_entity_t entity,
    ecs_id_t id)
{
    ecs_check(world != NULL, ECS_INVALID_PARAMETER, NULL);
    ecs_check(ecs_is_alive(world, entity), ECS_INVALID_PARAMETER, NULL);
    ecs_check(ecs_id_is_valid(world, id) || ecs_id_is_wildcard(id), 
        ECS_INVALID_PARAMETER, NULL);
    flecs_remove_id(world, entity, id);
error:
    return;
}

void ecs_auto_override_id(
    ecs_world_t *world,
    ecs_entity_t entity,
    ecs_id_t id)
{
    ecs_check(ecs_id_is_valid(world, id), ECS_INVALID_PARAMETER, NULL);
    ecs_add_id(world, entity, ECS_AUTO_OVERRIDE | id);
error:
    return;
}

ecs_entity_t ecs_clone(
    ecs_world_t *world,
    ecs_entity_t dst,
    ecs_entity_t src,
    bool copy_value)
{
    ecs_check(world != NULL, ECS_INVALID_PARAMETER, NULL);
    ecs_check(src != 0, ECS_INVALID_PARAMETER, NULL);
    ecs_check(ecs_is_alive(world, src), ECS_INVALID_PARAMETER, NULL);
    ecs_check(!dst || (ecs_get_table(world, dst)->type.count == 0), 
        ECS_INVALID_PARAMETER, NULL);

    ecs_stage_t *stage = flecs_stage_from_world(&world);
    if (!dst) {
        dst = ecs_new(world);
    }

    if (flecs_defer_clone(stage, dst, src, copy_value)) {
        return dst;
    }

    ecs_record_t *src_r = flecs_entities_get(world, src);
    ecs_assert(src_r != NULL, ECS_INTERNAL_ERROR, NULL);
    ecs_table_t *src_table = src_r->table;
    ecs_assert(src_table != NULL, ECS_INTERNAL_ERROR, NULL);

    ecs_table_t *dst_table = src_table;
    if (src_table->flags & EcsTableHasName) {
        dst_table = ecs_table_remove_id(world, src_table, 
            ecs_pair_t(EcsIdentifier, EcsName));
    }

    ecs_type_t dst_type = dst_table->type;
    ecs_table_diff_t diff = {
        .added = dst_type,
        .added_flags = dst_table->flags & EcsTableAddEdgeFlags
    };

    ecs_record_t *dst_r = flecs_entities_get(world, dst);

    if (dst_table != dst_r->table) {
        flecs_move_entity(world, dst, dst_r, dst_table, &diff, true, 0);
    }

    if (copy_value) {
        int32_t row = ECS_RECORD_TO_ROW(dst_r->row);
        int32_t i, count = src_table->column_count;
        for (i = 0; i < count; i ++) {
            int32_t type_id = ecs_table_column_to_type_index(src_table, i);
            ecs_id_t id = src_table->type.array[type_id];

            void *dst_ptr = ecs_get_mut_id(world, dst, id);
            if (!dst_ptr) {
                continue;
            }

            const void *src_ptr = ecs_get_id(world, src, id);
            const ecs_type_info_t *ti = src_table->data.columns[i].ti;
            if (ti->hooks.copy) {
                ti->hooks.copy(dst_ptr, src_ptr, 1, ti);
            } else {
                ecs_os_memcpy(dst_ptr, src_ptr, ti->size);
            }

            flecs_notify_on_set(world, dst_table, row, id, true);
        }

        if (dst_table->flags & EcsTableHasSparse) {
            count = dst_table->type.count;
            for (i = 0; i < count; i ++) {
                const ecs_table_record_t *tr = &dst_table->_->records[i];
                ecs_component_record_t *cr = (ecs_component_record_t*)tr->hdr.cache;
                if (cr->sparse) {
                    void *src_ptr = flecs_component_sparse_get(cr, src);
                    if (src_ptr) {
                        ecs_set_id(world, dst, cr->id, 0, src_ptr);
                    }
                }
            }
        }
    }

    if (src_r->row & EcsEntityHasDontFragment) {
        ecs_component_record_t *cur = world->cr_non_fragmenting_head;
        while (cur) {
            ecs_assert(cur->flags & EcsIdIsSparse, ECS_INTERNAL_ERROR, NULL);
            if (cur->sparse) {
                void *src_ptr = flecs_sparse_get(cur->sparse, 0, src);
                if (src_ptr) {
                    ecs_set_id(world, dst, cur->id, 0, src_ptr);
                }
            }
    
            cur = cur->non_fragmenting.next;
        }
    }

    flecs_defer_end(world, stage);
    return dst;
error:
    return 0;
}

#define ecs_get_low_id(table, r, id)\
    ecs_assert(table->component_map != NULL, ECS_INTERNAL_ERROR, NULL);\
    int16_t column_index = table->component_map[id];\
    if (column_index > 0) {\
        ecs_column_t *column = &table->data.columns[column_index - 1];\
        return ECS_ELEM(column->data, column->ti->size, \
            ECS_RECORD_TO_ROW(r->row));\
    }

const void* ecs_get_id(
    const ecs_world_t *world,
    ecs_entity_t entity,
    ecs_id_t id)
{
    ecs_check(world != NULL, ECS_INVALID_PARAMETER, NULL);
    ecs_check(ecs_is_alive(world, entity), ECS_INVALID_PARAMETER, NULL);

    world = ecs_get_world(world);

    ecs_record_t *r = flecs_entities_get(world, entity);
    ecs_assert(r != NULL, ECS_INVALID_PARAMETER, NULL);

    ecs_table_t *table = r->table;
    ecs_assert(table != NULL, ECS_INTERNAL_ERROR, NULL);

    if (id < FLECS_HI_COMPONENT_ID) {
        if (!world->non_trivial[id]) {
            ecs_get_low_id(table, r, id);
            return NULL;
        }
    }

    ecs_component_record_t *cr = flecs_components_get(world, id);
    if (!cr) {
        return NULL;
    }

    if (cr->flags & EcsIdDontFragment) {
        void *ptr = flecs_component_sparse_get(cr, entity);
        if (ptr) {
            return ptr;
        }
    }

    const ecs_table_record_t *tr = flecs_component_get_table(cr, table);
    if (!tr) {
        return flecs_get_base_component(world, table, id, cr, 0);
    } else {
        if (cr->flags & EcsIdIsSparse) {
            return flecs_component_sparse_get(cr, entity);
        }
        ecs_check(tr->column != -1, ECS_NOT_A_COMPONENT, NULL);
    }

    int32_t row = ECS_RECORD_TO_ROW(r->row);
    return flecs_table_get_component(table, tr->column, row).ptr;
error:
    return NULL;
}

void* ecs_get_mut_id(
    const ecs_world_t *world,
    ecs_entity_t entity,
    ecs_id_t id)
{
    ecs_check(world != NULL, ECS_INVALID_PARAMETER, NULL);
    ecs_check(ecs_is_alive(world, entity), ECS_INVALID_PARAMETER, NULL);

    world = ecs_get_world(world);

    flecs_check_exclusive_world_access_write(world);

    ecs_record_t *r = flecs_entities_get(world, entity);
    ecs_assert(r != NULL, ECS_INVALID_PARAMETER, NULL);

    if (id < FLECS_HI_COMPONENT_ID) {
        if (!world->non_trivial[id]) {
            ecs_get_low_id(r->table, r, id);
            return NULL;
        }
    }

    ecs_component_record_t *cr = flecs_components_get(world, id);
    int32_t row = ECS_RECORD_TO_ROW(r->row);
    return flecs_get_component_ptr(r->table, row, cr).ptr;
error:
    return NULL;
}

void* ecs_ensure_id(
    ecs_world_t *world,
    ecs_entity_t entity,
    ecs_id_t id)
{
    ecs_check(world != NULL, ECS_INVALID_PARAMETER, NULL);
    ecs_check(ecs_is_alive(world, entity), ECS_INVALID_PARAMETER, NULL);
    ecs_check(ecs_id_is_valid(world, id), ECS_INVALID_PARAMETER, NULL);

    ecs_stage_t *stage = flecs_stage_from_world(&world);
    if (flecs_defer_cmd(stage)) {
        return flecs_defer_set(
            world, stage, EcsCmdEnsure, entity, id, 0, NULL, NULL);
    }

    ecs_record_t *r = flecs_entities_get(world, entity);
    ecs_assert(r != NULL, ECS_INTERNAL_ERROR, NULL);
    void *result = flecs_ensure(world, entity, id, r).ptr;
    ecs_check(result != NULL, ECS_INVALID_PARAMETER, NULL);

    flecs_defer_end(world, stage);
    return result;
error:
    return NULL;
}

void* ecs_ensure_modified_id(
    ecs_world_t *world,
    ecs_entity_t entity,
    ecs_id_t id)
{
    ecs_check(world != NULL, ECS_INVALID_PARAMETER, NULL);
    ecs_check(ecs_is_alive(world, entity), ECS_INVALID_PARAMETER, NULL);
    ecs_check(ecs_id_is_valid(world, id), ECS_INVALID_PARAMETER, NULL);

    ecs_stage_t *stage = flecs_stage_from_world(&world);
    ecs_check(flecs_defer_cmd(stage), ECS_INVALID_PARAMETER, NULL);

    return flecs_defer_set(world, stage, EcsCmdSet, entity, id, 0, NULL, NULL);
error:
    return NULL;
}

void* ecs_get_mut_modified_id(
    ecs_world_t *world,
    ecs_entity_t entity,
    ecs_id_t id)
{
    ecs_check(world != NULL, ECS_INVALID_PARAMETER, NULL);
    ecs_check(ecs_is_alive(world, entity), ECS_INVALID_PARAMETER, NULL);
    ecs_check(ecs_id_is_valid(world, id), ECS_INVALID_PARAMETER, NULL);

    ecs_stage_t *stage = flecs_stage_from_world(&world);
    ecs_check(flecs_defer_cmd(stage), ECS_INVALID_PARAMETER, NULL);

    return flecs_defer_assign(world, stage, entity, id);
error:
    return NULL;
}

static
ecs_record_t* flecs_access_begin(
    ecs_world_t *stage,
    ecs_entity_t entity,
    bool write)
{
    ecs_check(ecs_os_has_threading(), ECS_MISSING_OS_API, NULL);

    const ecs_world_t *world = ecs_get_world(stage);
    ecs_record_t *r = flecs_entities_get(world, entity);
    ecs_assert(r != NULL, ECS_INTERNAL_ERROR, NULL);

    ecs_table_t *table = r->table;
    ecs_assert(table != NULL, ECS_INTERNAL_ERROR, NULL);

    int32_t count = ecs_os_ainc(&table->_->lock);
    (void)count;
    if (write) {
        ecs_check(count == 1, ECS_ACCESS_VIOLATION, NULL);
    }

    return r;
error:
    return NULL;
}

static
void flecs_access_end(
    const ecs_record_t *r,
    bool write)
{
    ecs_check(ecs_os_has_threading(), ECS_MISSING_OS_API, NULL);
    ecs_check(r != NULL, ECS_INVALID_PARAMETER, NULL);
    ecs_check(r->table != NULL, ECS_INVALID_PARAMETER, NULL);
    int32_t count = ecs_os_adec(&r->table->_->lock);
    (void)count;
    if (write) {
        ecs_check(count == 0, ECS_ACCESS_VIOLATION, NULL);
    }
    ecs_check(count >= 0, ECS_ACCESS_VIOLATION, NULL);

error:
    return;
}

ecs_record_t* ecs_write_begin(
    ecs_world_t *world,
    ecs_entity_t entity)
{
    return flecs_access_begin(world, entity, true);
}

void ecs_write_end(
    ecs_record_t *r)
{
    flecs_access_end(r, true);
}

const ecs_record_t* ecs_read_begin(
    ecs_world_t *world,
    ecs_entity_t entity)
{
    return flecs_access_begin(world, entity, false);
}

void ecs_read_end(
    const ecs_record_t *r)
{
    flecs_access_end(r, false);
}

ecs_entity_t ecs_record_get_entity(
    const ecs_record_t *record)
{
    ecs_check(record != NULL, ECS_INVALID_PARAMETER, NULL);
    ecs_table_t *table = record->table;
    ecs_assert(table != NULL, ECS_INTERNAL_ERROR, NULL);
    return table->data.entities[ECS_RECORD_TO_ROW(record->row)];
error:
    return 0;
}

const void* ecs_record_get_id(
    const ecs_world_t *stage,
    const ecs_record_t *r,
    ecs_id_t id)
{
    const ecs_world_t *world = ecs_get_world(stage);
    ecs_component_record_t *cr = flecs_components_get(world, id);
    return flecs_get_component(r->table, ECS_RECORD_TO_ROW(r->row), cr);
}

bool ecs_record_has_id(
    ecs_world_t *stage,
    const ecs_record_t *r,
    ecs_id_t id)
{
    const ecs_world_t *world = ecs_get_world(stage);
    if (r->table) {
        return ecs_table_has_id(world, r->table, id);
    }
    return false;
}

void* ecs_record_ensure_id(
    ecs_world_t *stage,
    ecs_record_t *r,
    ecs_id_t id)
{
    const ecs_world_t *world = ecs_get_world(stage);
    ecs_component_record_t *cr = flecs_components_get(world, id);
    return flecs_get_component(r->table, ECS_RECORD_TO_ROW(r->row), cr);
}

void* ecs_emplace_id(
    ecs_world_t *world,
    ecs_entity_t entity,
    ecs_id_t id,
    bool *is_new)
{
    ecs_check(world != NULL, ECS_INVALID_PARAMETER, NULL);
    ecs_check(ecs_is_alive(world, entity), ECS_INVALID_PARAMETER, NULL);
    ecs_check(ecs_id_is_valid(world, id), ECS_INVALID_PARAMETER, NULL);

    ecs_stage_t *stage = flecs_stage_from_world(&world);

    if (flecs_defer_cmd(stage)) {
        return flecs_defer_set(
            world, stage, EcsCmdEmplace, entity, id, 0, NULL, is_new);
    }

    ecs_check(is_new || !ecs_has_id(world, entity, id), ECS_INVALID_PARAMETER, 
        "cannot emplace a component the entity already has");

    ecs_component_record_t *cr = flecs_components_ensure(world, id);
    if (cr->flags & EcsIdDontFragment) {
        void *ptr = flecs_component_sparse_get(cr, entity);
        if (ptr) {
            if (is_new) {
                *is_new = false;
            }
            flecs_defer_end(world, stage);
            return ptr;
        }

        if (is_new) {
            *is_new = true;
        }
        is_new = NULL;
    }

    ecs_record_t *r = flecs_entities_get(world, entity);
    ecs_table_t *table = r->table;
    flecs_add_id_w_record(world, entity, r, id, false /* Add without ctor */);
    flecs_defer_end(world, stage);

    void *ptr = flecs_get_component(r->table, ECS_RECORD_TO_ROW(r->row), cr);
    ecs_check(ptr != NULL, ECS_INVALID_PARAMETER, 
        "emplaced component was removed during operation, make sure to not "
        "remove component T in on_add(T) hook/OnAdd(T) observer");

    if (is_new) {
        *is_new = table != r->table;
    }

    return ptr;
error:
    return NULL;
}

void flecs_modified_id_if(
    ecs_world_t *world,
    ecs_entity_t entity,
    ecs_id_t id,
    bool invoke_hook)
{
    ecs_check(world != NULL, ECS_INVALID_PARAMETER, NULL);
    ecs_check(ecs_is_alive(world, entity), ECS_INVALID_PARAMETER, NULL);
    ecs_check(ecs_id_is_valid(world, id), ECS_INVALID_PARAMETER, NULL);

    ecs_stage_t *stage = flecs_stage_from_world(&world);

    if (flecs_defer_modified(stage, entity, id)) {
        return;
    }

    ecs_record_t *r = flecs_entities_get(world, entity);
    ecs_table_t *table = r->table;
    ecs_assert(table != NULL, ECS_INTERNAL_ERROR, NULL);

    ecs_component_record_t *cr = flecs_components_get(world, id);
    if (!cr || !flecs_component_get_table(cr, table)) {
        flecs_defer_end(world, stage);
        return;
    }

    flecs_notify_on_set(
        world, table, ECS_RECORD_TO_ROW(r->row), id, invoke_hook);

    flecs_table_mark_dirty(world, table, id);
    flecs_defer_end(world, stage);
error:
    return;
}

void ecs_modified_id(
    ecs_world_t *world,
    ecs_entity_t entity,
    ecs_id_t id)
{
    ecs_check(world != NULL, ECS_INVALID_PARAMETER, NULL);
    ecs_check(ecs_is_alive(world, entity), ECS_INVALID_PARAMETER, NULL);
    ecs_check(ecs_id_is_valid(world, id), ECS_INVALID_PARAMETER, NULL);

    ecs_stage_t *stage = flecs_stage_from_world(&world);

    if (flecs_defer_modified(stage, entity, id)) {
        return;
    }

    /* If the entity does not have the component, calling ecs_modified is 
     * invalid. The assert needs to happen after the defer statement, as the
     * entity may not have the component when this function is called while
     * operations are being deferred. */
    ecs_check(ecs_has_id(world, entity, id), ECS_INVALID_PARAMETER, NULL);

    ecs_record_t *r = flecs_entities_get(world, entity);
    ecs_table_t *table = r->table;
    flecs_notify_on_set(world, table, ECS_RECORD_TO_ROW(r->row), id, true);

    flecs_table_mark_dirty(world, table, id);
    flecs_defer_end(world, stage);
error:
    return;
}

static
void flecs_set_id_copy(
    ecs_world_t *world,
    ecs_stage_t *stage,
    ecs_entity_t entity,
    ecs_id_t id,
    size_t size,
    void *ptr)
{
    if (flecs_defer_cmd(stage)) {
        flecs_defer_set(world, stage, EcsCmdSet, entity, id, 
            flecs_utosize(size), ptr, NULL);
        return;
    }

    ecs_record_t *r = flecs_entities_get(world, entity);
    flecs_component_ptr_t dst = flecs_ensure(world, entity, id, r);

    flecs_copy_id(world, r, id, size, dst.ptr, ptr, dst.ti);

    flecs_defer_end(world, stage);
}

void flecs_set_id_move(
    ecs_world_t *world,
    ecs_stage_t *stage,
    ecs_entity_t entity,
    ecs_id_t id,
    size_t size,
    void *ptr,
    ecs_cmd_kind_t cmd_kind)
{
    if (flecs_defer_cmd(stage)) {
        flecs_defer_set(world, stage, cmd_kind, entity, id, 
            flecs_utosize(size), ptr, NULL);
        return;
    }

    ecs_record_t *r = flecs_entities_get(world, entity);
    flecs_component_ptr_t dst = flecs_ensure(world, entity, id, r);
    ecs_check(dst.ptr != NULL, ECS_INVALID_PARAMETER, NULL);

    const ecs_type_info_t *ti = dst.ti;
    ecs_assert(ti != NULL, ECS_INTERNAL_ERROR, NULL);
    ecs_move_t move;
    if (cmd_kind != EcsCmdEmplace) {
        /* ctor will have happened by ensure */
        move = ti->hooks.move_dtor;
    } else {
        move = ti->hooks.ctor_move_dtor;
    }
    if (move) {
        move(dst.ptr, ptr, 1, ti);
    } else {
        ecs_os_memcpy(dst.ptr, ptr, flecs_utosize(size));
    }

    flecs_table_mark_dirty(world, r->table, id);

    if (cmd_kind == EcsCmdSet) {
        ecs_table_t *table = r->table;
        if (table->flags & EcsTableHasOnSet || ti->hooks.on_set) {
            ecs_type_t ids = { .array = &id, .count = 1 };
            flecs_notify_on_set_ids(
                world, table, ECS_RECORD_TO_ROW(r->row), 1, &ids);
        }
    }

    flecs_defer_end(world, stage);
error:
    return;
}

void ecs_set_id(
    ecs_world_t *world,
    ecs_entity_t entity,
    ecs_id_t id,
    size_t size,
    const void *ptr)
{
    ecs_check(world != NULL, ECS_INVALID_PARAMETER, NULL);
    ecs_check(ecs_is_alive(world, entity), ECS_INVALID_PARAMETER, NULL);
    ecs_check(ecs_id_is_valid(world, id), ECS_INVALID_PARAMETER, NULL);

    ecs_stage_t *stage = flecs_stage_from_world(&world);

    /* Safe to cast away const: function won't modify if move arg is false */
    flecs_set_id_copy(world, stage, entity, id, size, 
        ECS_CONST_CAST(void*, ptr));
error:
    return;
}

#if defined(FLECS_DEBUG) || defined(FLECS_KEEP_ASSERT)
static
bool flecs_can_toggle(
    ecs_world_t *world,
    ecs_id_t id)
{
    ecs_component_record_t *cr = flecs_components_get(world, id);
    if (!cr) {
        return false;
    }

    return (cr->flags & EcsIdCanToggle) != 0;
}
#endif

void ecs_enable_id(
    ecs_world_t *world,
    ecs_entity_t entity,
    ecs_id_t id,
    bool enable)
{
    ecs_check(world != NULL, ECS_INVALID_PARAMETER, NULL);
    ecs_stage_t *stage = flecs_stage_from_world(&world);

    ecs_check(ecs_is_valid(world, entity), ECS_INVALID_PARAMETER, NULL);
    ecs_check(ecs_id_is_valid(world, id), ECS_INVALID_PARAMETER, NULL);
    ecs_check(flecs_can_toggle(world, id), ECS_INVALID_OPERATION, 
        "add CanToggle trait to component");

    ecs_entity_t bs_id = id | ECS_TOGGLE;
    ecs_add_id((ecs_world_t*)stage, entity, bs_id);

    if (flecs_defer_enable(stage, entity, id, enable)) {
        return;
    }

    ecs_record_t *r = flecs_entities_get(world, entity);    
    ecs_table_t *table = r->table;
    int32_t index = ecs_table_get_type_index(world, table, bs_id);
    ecs_assert(index != -1, ECS_INTERNAL_ERROR, NULL);

    ecs_assert(table->_ != NULL, ECS_INTERNAL_ERROR, NULL);
    index -= table->_->bs_offset;
    ecs_assert(index >= 0, ECS_INTERNAL_ERROR, NULL);

    /* Data cannot be NULL, since entity is stored in the table */
    ecs_bitset_t *bs = &table->_->bs_columns[index];
    ecs_assert(bs != NULL, ECS_INTERNAL_ERROR, NULL);

    flecs_bitset_set(bs, ECS_RECORD_TO_ROW(r->row), enable);

    flecs_defer_end(world, stage);
error:
    return;
}

bool ecs_is_enabled_id(
    const ecs_world_t *world,
    ecs_entity_t entity,
    ecs_id_t id)
{
    ecs_check(world != NULL, ECS_INVALID_PARAMETER, NULL);
    ecs_check(ecs_is_alive(world, entity), ECS_INVALID_PARAMETER, NULL);
    ecs_check(ecs_id_is_valid(world, id), ECS_INVALID_PARAMETER, NULL);

    /* Make sure we're not working with a stage */
    world = ecs_get_world(world);

    ecs_record_t *r = flecs_entities_get(world, entity);
    ecs_assert(r != NULL, ECS_INTERNAL_ERROR, NULL);
    ecs_table_t *table = r->table;
    ecs_assert(table != NULL, ECS_INTERNAL_ERROR, NULL);

    ecs_entity_t bs_id = id | ECS_TOGGLE;
    int32_t index = ecs_table_get_type_index(world, table, bs_id);
    if (index == -1) {
        /* If table does not have TOGGLE column for component, component is
         * always enabled, if the entity has it */
        return ecs_has_id(world, entity, id);
    }

    ecs_assert(table->_ != NULL, ECS_INTERNAL_ERROR, NULL);
    index -= table->_->bs_offset;
    ecs_assert(index >= 0, ECS_INTERNAL_ERROR, NULL);
    ecs_bitset_t *bs = &table->_->bs_columns[index];

    return flecs_bitset_get(bs, ECS_RECORD_TO_ROW(r->row));
error:
    return false;
}

void ecs_set_child_order(
    ecs_world_t *world,
    ecs_entity_t parent,
    const ecs_entity_t *children,
    int32_t child_count)
{
    ecs_check(world != NULL, ECS_INVALID_PARAMETER, NULL);
    ecs_check(children == NULL || child_count, ECS_INVALID_PARAMETER, NULL);
    ecs_check(children != NULL || !child_count, ECS_INVALID_PARAMETER, NULL);
    ecs_check(!(world->flags & EcsWorldMultiThreaded), 
        ECS_INVALID_OPERATION, NULL);

    flecs_stage_from_world(&world);

    flecs_check_exclusive_world_access_write(world);

    ecs_component_record_t *cr = flecs_components_get(
        world, ecs_childof(parent));

    flecs_ordered_children_reorder(world, cr, children, child_count);

error:
    return;
}

ecs_entities_t ecs_get_ordered_children(
    const ecs_world_t *world,
    ecs_entity_t parent)
{
    ecs_check(world != NULL, ECS_INVALID_PARAMETER, NULL);

    world = ecs_get_world(world);

    flecs_check_exclusive_world_access_read(world);

    ecs_component_record_t *cr = flecs_components_get(
        world, ecs_childof(parent));
    ecs_check(cr != NULL, ECS_INVALID_PARAMETER, 
        "parent does not have the OrderedChildren trait");
    ecs_check(cr->flags & EcsIdOrderedChildren, ECS_INVALID_PARAMETER,
        "parent does not have the OrderedChildren trait");
    ecs_assert(cr->pair != NULL, ECS_INTERNAL_ERROR, NULL);

    return (ecs_entities_t){
        .count = ecs_vec_count(&cr->pair->ordered_children),
        .alive_count = ecs_vec_count(&cr->pair->ordered_children),
        .ids = ecs_vec_first(&cr->pair->ordered_children),
    };
error:
    return (ecs_entities_t){0};
}

bool ecs_has_id(
    const ecs_world_t *world,
    ecs_entity_t entity,
    ecs_id_t id)
{
    ecs_check(world != NULL, ECS_INVALID_PARAMETER, NULL);
    ecs_check(ecs_is_alive(world, entity), ECS_INVALID_PARAMETER, NULL);
    ecs_check(id != 0, ECS_INVALID_PARAMETER, NULL);

    /* Make sure we're not working with a stage */
    world = ecs_get_world(world);

    ecs_record_t *r = flecs_entities_get_any(world, entity);
    ecs_assert(r != NULL, ECS_INTERNAL_ERROR, NULL);
    ecs_table_t *table = r->table;
    ecs_assert(table != NULL, ECS_INTERNAL_ERROR, NULL);

    if (id < FLECS_HI_COMPONENT_ID) {
        if (!world->non_trivial[id]) {
            return table->component_map[id] != 0;
        }
    }

    ecs_component_record_t *cr = flecs_components_get(world, id);
    bool can_inherit = false;

    if (cr) {
        const ecs_table_record_t *tr = flecs_component_get_table(cr, table);
        if (tr) {
            return true;
        }

        if (cr->flags & (EcsIdDontFragment|EcsIdMatchDontFragment)) {
            if (flecs_component_sparse_has(cr, entity)) {
                return true;
            } else {
                return flecs_get_base_component(world, table, id, cr, 0) != NULL;
            }
        }

        can_inherit = cr->flags & EcsIdOnInstantiateInherit;
    }

    if (!(table->flags & EcsTableHasIsA)) {
        return false;
    }

    if (!can_inherit) {
        return false;
    }

    ecs_table_record_t *tr;
    int32_t column = ecs_search_relation(world, table, 0, id, 
        EcsIsA, 0, 0, 0, &tr);
    if (column == -1) {
        return false;
    }

    return true;
error:
    return false;
}

bool ecs_owns_id(
    const ecs_world_t *world,
    ecs_entity_t entity,
    ecs_id_t id)
{
    ecs_check(world != NULL, ECS_INVALID_PARAMETER, NULL);
    ecs_check(ecs_is_alive(world, entity), ECS_INVALID_PARAMETER, NULL);
    ecs_check(id != 0, ECS_INVALID_PARAMETER, NULL);

    /* Make sure we're not working with a stage */
    world = ecs_get_world(world);

    ecs_record_t *r = flecs_entities_get_any(world, entity);
    ecs_assert(r != NULL, ECS_INTERNAL_ERROR, NULL);
    ecs_table_t *table = r->table;
    ecs_assert(table != NULL, ECS_INTERNAL_ERROR, NULL);

    if (id < FLECS_HI_COMPONENT_ID) {
        if (!world->non_trivial[id]) {
            return table->component_map[id];
        }
    }

    ecs_component_record_t *cr = flecs_components_get(world, id);
    if (cr) {
        const ecs_table_record_t *tr = flecs_component_get_table(cr, table);
        if (tr) {
            return true;
        }

        if (cr->flags & (EcsIdDontFragment|EcsIdMatchDontFragment)) {
            return flecs_component_sparse_has(cr, entity);
        }
    }

error:
    return false;
}

ecs_entity_t ecs_get_target(
    const ecs_world_t *world,
    ecs_entity_t entity,
    ecs_entity_t rel,
    int32_t index)
{
    ecs_check(world != NULL, ECS_INVALID_PARAMETER, NULL);
    ecs_check(ecs_is_alive(world, entity), ECS_INVALID_PARAMETER, NULL);
    ecs_check(rel != 0, ECS_INVALID_PARAMETER, NULL);

    world = ecs_get_world(world);

    ecs_record_t *r = flecs_entities_get(world, entity);
    ecs_assert(r != NULL, ECS_INTERNAL_ERROR, NULL);
    ecs_table_t *table = r->table;
    ecs_assert(table != NULL, ECS_INTERNAL_ERROR, NULL);

    ecs_id_t wc = ecs_pair(rel, EcsWildcard);
    ecs_component_record_t *cr = flecs_components_get(world, wc);
    if (!cr) {
        return 0;
    }

    const ecs_table_record_t *tr = flecs_component_get_table(cr, table);;
    if (!tr) {
        if (cr->flags & EcsIdDontFragment) {
            if (cr->flags & EcsIdExclusive) {
                if (index > 0) {
                    return 0;
                }

                ecs_entity_t *tgt = flecs_component_sparse_get(cr, entity);
                if (tgt) {
                    return *tgt;
                }
            } else {
                ecs_type_t *type = flecs_component_sparse_get(cr, entity);
                if (type && (index < type->count)) {
                    return type->array[index];
                }
            }
        }

        if (cr->flags & EcsIdOnInstantiateInherit) {
            goto look_in_base;
        }

        return 0;
    }

    if (index >= tr->count) {
        index -= tr->count;
        goto look_in_base;
    }

    return ecs_pair_second(world, table->type.array[tr->index + index]);
look_in_base:
    if (table->flags & EcsTableHasIsA) {
        const ecs_table_record_t *tr_isa = flecs_component_get_table(
            world->cr_isa_wildcard, table);
        ecs_assert(tr_isa != NULL, ECS_INTERNAL_ERROR, NULL);

        ecs_id_t *ids = table->type.array;
        int32_t i = tr_isa->index, end = (i + tr_isa->count);
        for (; i < end; i ++) {
            ecs_id_t isa_pair = ids[i];
            ecs_entity_t base = ecs_pair_second(world, isa_pair);
            ecs_assert(base != 0, ECS_INTERNAL_ERROR, NULL);
            ecs_entity_t t = ecs_get_target(world, base, rel, index);
            if (t) {
                return t;
            }
        }
    }

error:
    return 0;
}

ecs_entity_t ecs_get_parent(
    const ecs_world_t *world,
    ecs_entity_t entity)
{
    ecs_check(world != NULL, ECS_INVALID_PARAMETER, NULL);
    ecs_check(ecs_is_alive(world, entity), ECS_INVALID_PARAMETER, NULL);

    world = ecs_get_world(world);

    ecs_record_t *r = flecs_entities_get(world, entity);
    ecs_assert(r != NULL, ECS_INTERNAL_ERROR, NULL);
    ecs_table_t *table = r->table;
    ecs_assert(table != NULL, ECS_INTERNAL_ERROR, NULL);

    ecs_component_record_t *cr = world->cr_childof_wildcard;
    ecs_assert(cr != NULL, ECS_INTERNAL_ERROR, NULL);

    const ecs_table_record_t *tr = flecs_component_get_table(cr, table);
    if (!tr) {
        return 0;
    }

    ecs_entity_t id = table->type.array[tr->index];
    return flecs_entities_get_alive(world, ECS_PAIR_SECOND(id));
error:
    return 0;
}

ecs_entity_t ecs_get_target_for_id(
    const ecs_world_t *world,
    ecs_entity_t entity,
    ecs_entity_t rel,
    ecs_id_t id)
{
    ecs_check(world != NULL, ECS_INVALID_PARAMETER, NULL);
    ecs_check(ecs_is_alive(world, entity), ECS_INVALID_PARAMETER, NULL);

    if (!id) {
        return ecs_get_target(world, entity, rel, 0);
    }

    world = ecs_get_world(world);

    ecs_table_t *table = ecs_get_table(world, entity);
    ecs_entity_t subject = 0;

    if (rel) {
        int32_t column = ecs_search_relation(
            world, table, 0, id, rel, 0, &subject, 0, 0);
        if (column == -1) {
            return 0;
        }
    } else {
        entity = 0; /* Don't return entity if id was not found */

        if (table) {
            ecs_id_t *ids = table->type.array;
            int32_t i, count = table->type.count;

            for (i = 0; i < count; i ++) {
                ecs_id_t ent = ids[i];
                if (ent & ECS_ID_FLAGS_MASK) {
                    /* Skip ids with pairs, roles since 0 was provided for rel */
                    break;
                }

                if (ecs_has_id(world, ent, id)) {
                    subject = ent;
                    break;
                }
            }
        }
    }

    if (subject == 0) {
        return entity;
    } else {
        return subject;
    }
error:
    return 0;
}

int32_t ecs_get_depth(
    const ecs_world_t *world,
    ecs_entity_t entity,
    ecs_entity_t rel)
{
    ecs_check(ecs_is_valid(world, rel), ECS_INVALID_PARAMETER, NULL);
    ecs_check(ecs_is_alive(world, entity), ECS_INVALID_PARAMETER, NULL);
    ecs_check(ecs_has_id(world, rel, EcsAcyclic), ECS_INVALID_PARAMETER, 
        "cannot safely determine depth for relationship that is not acyclic "
            "(add Acyclic property to relationship)");

    ecs_table_t *table = ecs_get_table(world, entity);
    if (table) {
        return ecs_table_get_depth(world, table, rel);
    }

    return 0;
error:
    return -1;
}

bool ecs_is_valid(
    const ecs_world_t *world,
    ecs_entity_t entity)
{
    ecs_check(world != NULL, ECS_INVALID_PARAMETER, NULL);

    #ifdef FLECS_DEBUG
    world = ecs_get_world(world);
    flecs_check_exclusive_world_access_read(world);
    #endif

    /* 0 is not a valid entity id */
    if (!entity) {
        return false;
    }
    
    /* Entity identifiers should not contain flag bits */
    if (entity & ECS_ID_FLAGS_MASK) {
        return false;
    }

    /* Entities should not contain data in dead zone bits */
    if (entity & ~0xFF00FFFFFFFFFFFF) {
        return false;
    }

    /* If id exists, it must be alive (the generation count must match) */
    return ecs_is_alive(world, entity);
error:
    return false;
}

ecs_id_t ecs_strip_generation(
    ecs_entity_t e)
{
    /* If this is not a pair, erase the generation bits */
    if (!(e & ECS_ID_FLAGS_MASK)) {
        e &= ~ECS_GENERATION_MASK;
    }

    return e;
}

bool ecs_is_alive(
    const ecs_world_t *world,
    ecs_entity_t entity)
{
    ecs_check(world != NULL, ECS_INVALID_PARAMETER, NULL);
    ecs_check(entity != 0, ECS_INVALID_PARAMETER, NULL);

    world = ecs_get_world(world);

    flecs_check_exclusive_world_access_read(world);

    return flecs_entities_is_alive(world, entity);
error:
    return false;
}

ecs_entity_t ecs_get_alive(
    const ecs_world_t *world,
    ecs_entity_t entity)
{
    ecs_check(world != NULL, ECS_INVALID_PARAMETER, NULL);
    
    if (!entity) {
        return 0;
    }

    /* Make sure we're not working with a stage */
    world = ecs_get_world(world);

    flecs_check_exclusive_world_access_read(world);

    if (flecs_entities_is_alive(world, entity)) {
        return entity;
    }

    /* Make sure id does not have generation. This guards against accidentally
     * "upcasting" a not alive identifier to an alive one. */
    if ((uint32_t)entity != entity) {
        return 0;
    }

    ecs_entity_t current = flecs_entities_get_alive(world, entity);
    if (!current || !flecs_entities_is_alive(world, current)) {
        return 0;
    }

    return current;
error:
    return 0;
}

void ecs_make_alive(
    ecs_world_t *world,
    ecs_entity_t entity)
{
    ecs_assert(world != NULL, ECS_INTERNAL_ERROR, NULL);
    ecs_check(entity != 0, ECS_INVALID_PARAMETER, NULL);

    /* Const cast is safe, function checks for threading */
    world = ECS_CONST_CAST(ecs_world_t*, ecs_get_world(world));

    flecs_check_exclusive_world_access_write(world);

    /* The entity index can be mutated while in staged/readonly mode, as long as
     * the world is not multithreaded. */
    ecs_assert(!(world->flags & EcsWorldMultiThreaded), 
        ECS_INVALID_OPERATION, 
            "cannot make entity alive while world is in multithreaded mode");

    /* Check if a version of the provided id is alive */
    ecs_entity_t any = ecs_get_alive(world, (uint32_t)entity);
    if (any == entity) {
        /* If alive and equal to the argument, there's nothing left to do */
        return;
    }

    /* If the id is currently alive but did not match the argument, fail */
    ecs_check(!any, ECS_INVALID_PARAMETER, 
        "entity is alive with different generation");

    /* Set generation if not alive. The sparse set checks if the provided
     * id matches its own generation which is necessary for alive ids. This
     * check would cause ecs_ensure to fail if the generation of the 'entity'
     * argument doesn't match with its generation.
     * 
     * While this could've been addressed in the sparse set, this is a rare
     * scenario that can only be triggered by ecs_ensure. Implementing it here
     * allows the sparse set to not do this check, which is more efficient. */
    flecs_entities_make_alive(world, entity);

    /* Ensure id exists. The underlying data structure will verify that the
     * generation count matches the provided one. */
    ecs_record_t *r = flecs_entities_ensure(world, entity);
    ecs_assert(r != NULL, ECS_INTERNAL_ERROR, NULL);
    ecs_assert(r->table == NULL, ECS_INTERNAL_ERROR, NULL);

    ecs_table_diff_t diff = ECS_TABLE_DIFF_INIT;
    flecs_new_entity(world, entity, r, &world->store.root, &diff, false, 0);
    ecs_assert(r->table == &world->store.root, ECS_INTERNAL_ERROR, NULL);
error:
    return;
}

void ecs_make_alive_id(
    ecs_world_t *world,
    ecs_id_t id)
{
    flecs_poly_assert(world, ecs_world_t);
    if (ECS_HAS_ID_FLAG(id, PAIR)) {
        ecs_entity_t r = ECS_PAIR_FIRST(id);
        ecs_entity_t o = ECS_PAIR_SECOND(id);

        ecs_check(r != 0, ECS_INVALID_PARAMETER, NULL);
        ecs_check(o != 0, ECS_INVALID_PARAMETER, NULL);

        if (flecs_entities_get_alive(world, r) == 0) {
            ecs_assert(!ecs_exists(world, r), ECS_INVALID_PARAMETER, 
                "first element of pair is not alive");
            ecs_make_alive(world, r);
        }
        if (flecs_entities_get_alive(world, o) == 0) {
            ecs_assert(!ecs_exists(world, o), ECS_INVALID_PARAMETER,
                "second element of pair is not alive");
                ecs_make_alive(world, o);
        }
    } else {
        ecs_make_alive(world, id & ECS_COMPONENT_MASK);
    }
error:
    return;
}

bool ecs_exists(
    const ecs_world_t *world,
    ecs_entity_t entity)
{
    ecs_check(world != NULL, ECS_INVALID_PARAMETER, NULL);
    ecs_check(entity != 0, ECS_INVALID_PARAMETER, NULL);

    world = ecs_get_world(world);

    flecs_check_exclusive_world_access_read(world);

    return flecs_entities_exists(world, entity);
error:
    return false;
}

void ecs_set_version(
    ecs_world_t *world,
    ecs_entity_t entity_with_generation)
{
    flecs_poly_assert(world, ecs_world_t);
    ecs_assert(!(world->flags & EcsWorldReadonly), ECS_INVALID_OPERATION,
        "cannot change entity generation when world is in readonly mode");
    ecs_assert(!(ecs_is_deferred(world)), ECS_INVALID_OPERATION, 
        "cannot change entity generation while world is deferred");

    flecs_check_exclusive_world_access_write(world);

    flecs_entities_make_alive(world, entity_with_generation);

    if (flecs_entities_is_alive(world, entity_with_generation)) {
        ecs_record_t *r = flecs_entities_get(world, entity_with_generation);
        if (r && r->table) {
            int32_t row = ECS_RECORD_TO_ROW(r->row);
            ecs_entity_t *entities = r->table->data.entities;
            entities[row] = entity_with_generation;
        }
    }
}

ecs_table_t* ecs_get_table(
    const ecs_world_t *world,
    ecs_entity_t entity)
{
    ecs_check(world != NULL, ECS_INVALID_PARAMETER, NULL);
    ecs_check(ecs_is_valid(world, entity), ECS_INVALID_PARAMETER, NULL);
    
    world = ecs_get_world(world);

    flecs_check_exclusive_world_access_read(world);

    ecs_record_t *record = flecs_entities_get(world, entity);
    ecs_assert(record != NULL, ECS_INTERNAL_ERROR, NULL);
    return record->table;
error:
    return NULL;
}

const ecs_type_t* ecs_get_type(
    const ecs_world_t *world,
    ecs_entity_t entity)
{
    ecs_table_t *table = ecs_get_table(world, entity);
    if (table) {
        return &table->type;
    }

    return NULL;
}

void ecs_enable(
    ecs_world_t *world,
    ecs_entity_t entity,
    bool enabled)
{
    if (ecs_has_id(world, entity, EcsPrefab)) {
        /* If entity is a type, enable/disable all entities in the type */
        const ecs_type_t *type = ecs_get_type(world, entity);
        ecs_assert(type != NULL, ECS_INTERNAL_ERROR, NULL);
        ecs_id_t *ids = type->array;
        int32_t i, count = type->count;
        for (i = 0; i < count; i ++) {
            ecs_id_t id = ids[i];
            if (!(ecs_id_get_flags(world, id) & EcsIdOnInstantiateDontInherit)){
                ecs_enable(world, id, enabled);
            }
        }
    } else {
        if (enabled) {
            ecs_remove_id(world, entity, EcsDisabled);
        } else {
            ecs_add_id(world, entity, EcsDisabled);
        }
    }
}

static
void ecs_type_str_buf(
    const ecs_world_t *world,
    const ecs_type_t *type,
    ecs_strbuf_t *buf)
{
    ecs_entity_t *ids = type->array;
    int32_t i, count = type->count;

    for (i = 0; i < count; i ++) {
        ecs_entity_t id = ids[i];

        if (i) {
            ecs_strbuf_appendch(buf, ',');
            ecs_strbuf_appendch(buf, ' ');
        }

        if (id == 1) {
            ecs_strbuf_appendlit(buf, "Component");
        } else {
            ecs_id_str_buf(world, id, buf);
        }
    }
}

char* ecs_type_str(
    const ecs_world_t *world,
    const ecs_type_t *type)
{
    if (!type) {
        return ecs_os_strdup("");
    }

    ecs_strbuf_t buf = ECS_STRBUF_INIT;
    ecs_type_str_buf(world, type, &buf);
    return ecs_strbuf_get(&buf);
}

char* ecs_entity_str(
    const ecs_world_t *world,
    ecs_entity_t entity)
{
    ecs_strbuf_t buf = ECS_STRBUF_INIT;
    ecs_check(ecs_is_alive(world, entity), ECS_INVALID_PARAMETER, NULL);

    ecs_get_path_w_sep_buf(world, 0, entity, 0, "", &buf, false);
    
    ecs_strbuf_appendlit(&buf, " [");
    const ecs_type_t *type = ecs_get_type(world, entity);
    if (type) {
        ecs_type_str_buf(world, type, &buf);
    }
    ecs_strbuf_appendch(&buf, ']');

    return ecs_strbuf_get(&buf);
error:
    return NULL;
}

