#pragma once

#include "flecs.h"
#include <unordered_map>
#include <vector>

namespace octopus
{

template<typename Key, typename Value>
struct Entry
{
    Key key;
    Value val;
};

template<typename Key, typename Value>
struct fast_map;

template <typename Key, typename Value, typename Elem = Entry<Key, Value>, typename Map = fast_map<Key, Value>>
flecs::opaque<Map, Elem> fast_map_support(flecs::world& world);

template<typename Key, typename Value>
struct fast_map
{
    fast_map() = default;
    fast_map(std::unordered_map<Key, Value> const &map_p) : map(map_p) {}

    Value &operator[](Key const key)
    {
        set_up();
        return map[key];
    }
    Value const &operator[](Key const key) const
    {
        set_up();
        return map.at(key);
    }

    std::unordered_map<Key, Value> const &data() const
    {
        set_up();
        return map;
    }

    std::unordered_map<Key, Value> &data()
    {
        set_up();
        return map;
    }

private:
    mutable std::unordered_map<Key, Value> map;
    mutable std::vector<Entry<Key, Value> > vector;

    void set_up() const
    {
        if(!vector.empty())
        {
            assert(map.empty());
            for(Entry<Key,Value> &entry : vector)
            {
                map[entry.key] = entry.val;
            }
            vector.clear();
        }
    }

    friend flecs::opaque<fast_map<Key, Value>, Entry<Key, Value>> fast_map_support<Key, Value>(flecs::world& world);
};

// Reusable reflection support for fast map
template <typename Key, typename Value, typename Elem, typename Map>
flecs::opaque<Map, Elem> fast_map_support(flecs::world& world) {
    return flecs::opaque<Map, Elem>()
        .as_type(world.vector<Elem>())

        // Forward elements of fast map value to serializer
        .serialize([](const flecs::serializer *s, const Map *data) {
            data->set_up();
            for (const auto& el : data->map) {
                s->value(Elem {el.first, el.second});
            }
            return 0;
        })

        // Return Map count
        .count([](const Map *data) {
            return data->map.size();
        })

        // Resize contents of Map
        .resize([](Map *data, size_t size) {
            data->vector.resize(size);
        })

        // Ensure element exists, return pointer
        .ensure_element([](Map *data, size_t elem) {
            if (data->vector.size() <= elem) {
                data->vector.resize(elem + 1);
            }

            return &data->vector.data()[elem];
        })
    ;
}

} // namespace octopus
