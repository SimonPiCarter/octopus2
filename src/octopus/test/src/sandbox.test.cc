#include <gtest/gtest.h>

#include "flecs.h"
#include <iostream>
#include <string>
#include <vector>
#include <variant>

// Use opaque reflection support to add a computed 'result' member to type
struct Sum {
    int32_t a;
    int32_t b;
};

struct Point {
    float x;
    float y;
};

struct Line {
    Point start;
    Point stop;
};

struct Variant {
	std::variant<Line, Sum> var;
};


// Reusable reflection support for std::vector
template<typename variant_t>
void variant_support(flecs::world& world) {
	world.component<variant_t>()
        // Serialize as struct
        .opaque(world.component()
            .member<Line>("line")
            .member<Sum>("sum"))
        // Forward struct members to serializer
        .serialize([](const flecs::serializer *s, const variant_t *data) {
			if(std::holds_alternative<Line>(data->var))
			{
				s->member("line");
            	s->value(std::get<Line>(data->var));
			}
			else if(std::holds_alternative<Sum>(data->var))
			{
				s->member("sum");
            	s->value(std::get<Sum>(data->var));
			}
            return 0;
        })

        // Return address for requested member
        .ensure_member([](variant_t *dst, const char *member) -> void* {
            if (!strcmp(member, "line")) {
				dst->var = Line();
                return &std::get<Line>(dst->var);
            } else if (!strcmp(member, "sum")) {
				dst->var = Sum();
                return &std::get<Sum>(dst->var);
            } else {
                return nullptr; // We can't serialize into fake result member
            }
        });
}

TEST(sandbox, test)
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

	variant_support<Variant>(ecs);

    // Serialize value of Sum to JSON
    Sum v = {10, 20};
    std::cout << ecs.to_json(&v) << std::endl;

    // Deserialize new value into Sum
    ecs.from_json(&v, "{\"a\": 20, \"b\": 22}");

    // Serialize value again
    std::cout << ecs.to_json(&v) << std::endl;

    // Create entity with Line as usual
    flecs::entity e = ecs.entity()
        .set<Line>({{10, 20}, {30, 40}});

    // Convert Line component to flecs expression string
    const Line *ptr = e.get<Line>();
    std::cout << ecs.to_expr(ptr).c_str() << std::endl;

    Variant var_l;
	var_l.var = *ptr;
    std::cout << ecs.to_json(&var_l) << std::endl;

	var_l.var = v;
    std::cout << ecs.to_json(&var_l) << std::endl;

    ecs.from_json(&var_l, "{\"line\":{\"start\":{\"x\":10, \"y\":20}, \"stop\":{\"x\":30, \"y\":40}}}");

    std::cout << ecs.to_json(&var_l) << std::endl;

    // Output
    //  {"a":10, "b":20, "result":30}
    //  {"a":22, "b":20, "result":42}
    // {start: {x: 10.00, y: 20.00}, stop: {x: 30.00, y: 40.00}}
}
