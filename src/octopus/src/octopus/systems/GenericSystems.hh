#pragma once

#include <flecs.h>

namespace octopus
{

template<typename T>
MementoQuery<T> setup_generic_systems_for_memento(flecs::world &ecs, flecs::world &ecs_step)
{
    MementoQuery<T> queries = create_memento_query<T>(ecs, ecs_step);

    create_applying_system<T>(ecs);
    create_reverting_system<T>(ecs);

    return queries;
}

} // octopus
