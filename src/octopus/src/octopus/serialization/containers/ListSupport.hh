#pragma once

#include <list>
#include <flecs.h>

// Reusable reflection support for std::vector
template <typename Elem, typename List = std::list<Elem>>
flecs::opaque<List, Elem> std_list_support(flecs::world& world) {
    return flecs::opaque<List, Elem>()
        .as_type(world.vector<Elem>())

        // Forward elements of std::vector value to serializer
        .serialize([](const flecs::serializer *s, const List *data) {
            for (const auto& el : *data) {
                s->value(el);
            }
            return 0;
        })

        // Return list count
        .count([](const List *data) {
            return data->size();
        })

        // Resize contents of list
        .resize([](List *data, size_t size) {
            data->resize(size);
        })

        // Ensure element exists, return pointer
        .ensure_element([](List *data, size_t elem) {
            if (data->size() <= elem) {
                data->resize(elem + 1);
				return &data->back();
            }

            return &(*std::next(data->begin(), elem));
        });
}
