#include <gtest/gtest.h>

#include <flecs.h>
#include <iostream>
#include <string>
#include <vector>

#include "octopus/serialization/containers/VectorSupport.hh"

// This example shows how to serialize a component with std::vectors

struct VectorComponent {
    std::vector<int32_t> ints;
    std::vector<std::string> strings;
};

TEST(ser_vector, test)
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

    // Register reflection for std::vector<int>
    ecs.component<std::vector<int>>()
        .opaque(std_vector_support<int>);

    // Register reflection for std::vector<std::string>
    ecs.component<std::vector<std::string>>()
        .opaque(std_vector_support<std::string>);

    // Register component with std::vector members
    ecs.component<VectorComponent>()
        .member<std::vector<int>>("ints")
        .member<std::vector<std::string>>("strings");

    // Create value & serialize it to JSON
    VectorComponent v = {{1, 2, 3}, {"foo", "bar"}};
    std::cout << ecs.to_json(&v) << std::endl;

    // Deserialize new values from JSON into value
    ecs.from_json(&v,
        "{\"ints\": [4, 5], \"strings\":[\"hello\", \"flecs\", \"reflection\"]}");

    // Serialize again
    std::cout << ecs.to_json(&v) << std::endl;
}
