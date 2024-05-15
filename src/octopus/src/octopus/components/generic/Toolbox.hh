#pragma once

#include "flecs.h"

namespace octopus
{

///////
/////// Generic
///////

/// @brief helper method to get memento type from data component type
template<typename T>
typename T::Memento memento(T const &)
{
    return typename T::Memento();
}

template<typename... T>
flecs::entity add(flecs::world &ecs, flecs::world &step, T... data)
{
    flecs::entity ent_l = ecs.entity();
    flecs::entity ent_s_l = step.entity();

    ([&]
    {
        ent_l.set(data);
        ent_l.set(memento(data));
        ent_s_l.set(memento(data));
    } (), ...);

    return ent_l;
}

template<typename T>
flecs::query<T const, typename T::Memento> create_working_query(flecs::world &ecs)
{
    return ecs.query<T const, typename T::Memento>();
}

template<typename T>
flecs::query<T, typename T::Memento const> create_stepping_query(flecs::world &ecs)
{
    return ecs.query<T, typename T::Memento const>();
}

/// @brief create a system that will apply changes to every components in the state
template<typename T>
void create_applying_system(flecs::world &ecs)
{
    // applying changes
    auto sys = ecs.system<T, typename T::Memento const>().multi_threaded();
    sys.template kind<Apply>();
    sys.each([](T& p, typename T::Memento const &v) {
            apply(p, v);
        });
}

/// @brief create a system that will revert changes to every components in the state
template<typename T>
void create_reverting_system(flecs::world &ecs)
{
    // applying changes
    auto sys = ecs.system<T, typename T::Memento const>().multi_threaded();
    sys.template kind<Revert>();
    sys.each([](T& p, typename T::Memento const &v) {
            revert(p, v);
        });
}

template<typename T>
struct MementoQuery
{
    flecs::query<typename T::Memento> q_data;
    flecs::query<typename T::Memento> q_step;

    void register_to_step()
    {
        iter([this](size_t const &count, typename T::Memento *data, typename T::Memento *step){
            for (size_t i = 0; i < count; i ++) {
                std::swap(step[i], data[i]);
                set_no_op(data[i]);
            }
        });
    }

    void register_from_step()
    {
        iter([this](size_t const &count, typename T::Memento *data, typename T::Memento *step){
            for (size_t i = 0; i < count; i ++) {
                data[i] = step[i];
            }
        });
    }

private:
    template<typename Func>
    void iter(Func && func_p)
    {
        q_data.iter([this, &func_p](flecs::iter& it, typename T::Memento *data) {
            q_step.iter([&it, &data, &func_p](flecs::iter& it2, typename T::Memento *step) {
                func_p(std::min(it.count(), it2.count()), data, step);
            });
        });
    }
};

template<typename T>
MementoQuery<T> create_memento_query(flecs::world &ecs, flecs::world &step)
{
    MementoQuery<T> queries;
    queries.q_data = ecs.query<typename T::Memento>();
    queries.q_step = step.query<typename T::Memento>();
    return queries;
}

} // octopus
