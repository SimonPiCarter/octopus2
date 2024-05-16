#include <gtest/gtest.h>

#include <flecs.h>
#include <iostream>
#include <string>
#include <list>

#include "octopus/serialization/containers/ListSupport.hh"

// This example shows how to serialize a component with std::lists

struct ListComponent {
    std::list<int32_t> ints;
    std::list<std::string> strings;
};

TEST(ser_list, test)
{
    flecs::world ecs;

    // Register reflection for std::string
    ecs.component<std::string>()
        .opaque(flecs::String) // Maps to string
            .serialize([](const flecs::serializer *s, const std::string *data) {
                const char *str = data->c_str();
                return s->value(flecs::String, &str); // Forward to serializer
            })
            .assign_string([](std::string* data, const char *value) {
                *data = value; // Assign new value to std::string
            });

    // Register reflection for std::list<int>
    ecs.component<std::list<int>>()
        .opaque(std_list_support<int>);

    // Register reflection for std::list<std::string>
    ecs.component<std::list<std::string>>()
        .opaque(std_list_support<std::string>);

    // Register component with std::list members
    ecs.component<ListComponent>()
        .member<std::list<int>>("ints")
        .member<std::list<std::string>>("strings");

    // Create value & serialize it to JSON
    ListComponent v = {{1, 2, 3}, {"foo", "bar"}};
    std::cout << ecs.to_json(&v) << std::endl;

    // Deserialize new values from JSON into value
    ecs.from_json(&v,
        "{\"ints\": [4, 5], \"strings\":[\"hello\", \"flecs\", \"reflection\"]}");

    // Serialize again
    std::cout << ecs.to_json(&v) << std::endl;
}
