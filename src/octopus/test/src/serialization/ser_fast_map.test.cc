#include <gtest/gtest.h>

#include "flecs.h"
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <variant>

#include "octopus/systems/phases/Phases.hh"
#include "octopus/components/step/StepContainer.hh"
#include "octopus/serialization/containers/VectorSupport.hh"
#include "octopus/utils/fast_map/fast_map.hh"

using namespace octopus;

// Some demo components:

struct FastMapComponent
{
    fast_map<int, std::string> map;
};

TEST(set_fast_map, simple)
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

    ecs.component<Entry<int, std::string>>()
        .member("key", &Entry<int, std::string>::key)
        .member("val", &Entry<int, std::string>::val);

    ecs.component<fast_map<int, std::string> >()
        .opaque(fast_map_support<int, std::string>);

    // Register component with std::vector members
    ecs.component<FastMapComponent>()
        .member("map", &FastMapComponent::map);

    FastMapComponent ref;
    ref.map[1] = "ert";
    ref.map[4] = "pcp";

    std::cout<< ecs.to_json(&ref) << std::endl;

    FastMapComponent rec;
    ecs.from_json(&rec, ecs.to_json(&ref));

    std::cout<< ecs.to_json(&rec) << std::endl;
}
