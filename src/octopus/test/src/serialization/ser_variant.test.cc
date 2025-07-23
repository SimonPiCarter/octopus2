#include <gtest/gtest.h>

#include "flecs.h"
#include <iostream>
#include <string>
#include <vector>
#include <variant>

#include "octopus/serialization/variant/VariantSupport.hh"

// Use opaque reflection support to add a computed 'result' member to type
struct Sum {
    int32_t a;
    int32_t b;

	constexpr static char const * naming() { return "sum"; }
};

struct Point {
    float x;
    float y;

	constexpr static char const * naming() { return "point"; }
};

struct Line {
    Point start;
    Point stop;

	constexpr static char const * naming() { return "line"; }
};

struct Variant {
	std::variant<Line, Sum, Point> var;
};

TEST(ser_variant, test)
{
    flecs::world ecs;

    // Register components with reflection data
    ecs.component<Point>()
        .member<float>("x")
        .member<float>("y");

    ecs.component<Line>()
        .member<Point>("start")
        .member<Point>("stop");

    // Register serialization support for opaque type
    ecs.component<Sum>()
        // Serialize as struct
        .opaque(ecs.component()
            .member<int32_t>("a")
            .member<int32_t>("b"))

        // Forward struct members to serializer
        .serialize([](const flecs::serializer *s, const Sum *data) {
            s->member("x");
            s->value(data->a);
            s->member("y");
            s->value(data->b);

            s->member("result");
            s->value(data->a + data->b); // Serialize fake member
            return 0;
        })

        // Return address for requested member
        .ensure_member([](Sum *dst, const char *member) -> void* {
            if (!strcmp(member, "a")) {
                return &dst->a;
            } else if (!strcmp(member, "b")) {
                return &dst->b;
            } else {
                return &dst->b; // We can't serialize into fake result member
            }
        });

    // Serialize value of Sum to JSON
    Sum v = {10, 20};
    // Deserialize new value into Sum
    ecs.from_json(&v, "{\"a\": 20, \"b\": 22}");

    // Create entity with Line as usual
    flecs::entity e = ecs.entity()
        .set<Line>({{10, 20}, {30, 40}});

    // Convert Line component to flecs expression string
    const Line *ptr = e.try_get<Line>();

	variant_support<Line, Sum, Point>(ecs);

    ecs.component<Variant>()
		.member<std::variant<Line, Sum, Point> >("var");

    Variant var_l;
	var_l.var = *ptr;
    std::cout << ecs.to_json(&var_l) << std::endl;

	var_l.var = v;
    std::cout << ecs.to_json(&var_l) << std::endl;

    ecs.from_json(&var_l, "{\"var\":{\"line\":{\"start\":{\"x\":10, \"y\":20}, \"stop\":{\"x\":30, \"y\":40}}}}");
	EXPECT_TRUE(std::holds_alternative<Line>(var_l.var));

    std::cout << ecs.to_json(&var_l) << std::endl;

    ecs.from_json(&var_l, "{\"var\":{\"point\":{\"x\":12, \"y\":20}}}}");

    std::cout << ecs.to_json(&var_l) << std::endl;

	EXPECT_TRUE(std::holds_alternative<Point>(var_l.var));

    // Output
    //  {"a":10, "b":20, "result":30}
    //  {"a":22, "b":20, "result":42}
    // {start: {x: 10.00, y: 20.00}, stop: {x: 30.00, y: 40.00}}
}
