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

template<typename type_t>
void add_member(flecs::untyped_component &component_p)
{
	component_p.member<type_t>(type_t::naming());
}

template<typename variant_t, typename type_t>
void serialize_member(const flecs::serializer *s, const variant_t *data)
{
	if(std::holds_alternative<type_t>(*data))
	{
		s->member(type_t::naming());
		s->value(std::get<type_t>(*data));
	}
}

template<typename variant_t, typename type_t>
void ensure_member(void *& return_r, const char *member, variant_t *data)
{
	if (!strcmp(member, type_t::naming())) {
		*data = type_t();
		return_r = &std::get<type_t>(*data);
	}
}

// Reusable reflection support for std::vector
template<typename... tArgs>
void variant_support(flecs::world& world) {

	flecs::untyped_component comp_l = world.component();
	// cf https://stackoverflow.com/questions/12515616/expression-contains-unexpanded-parameter-packs/12515637#12515637
    int _[] = {0, (add_member<tArgs>(comp_l), 0)...}; (void)_;

	using variant_t = std::variant<tArgs...>;

	world.component<variant_t>()
        // Serialize as struct
        .opaque(comp_l)
        // Forward struct members to serializer
        .serialize([](const flecs::serializer *s, const variant_t *data) {
			int _[] = {0, (serialize_member<variant_t, tArgs>(s, data), 0)...}; (void)_;
            return 0;
        })

        // Return address for requested member
        .ensure_member([](variant_t *dst, const char *member) -> void* {
			void* return_l = nullptr;
			int _[] = {0, (ensure_member<variant_t, tArgs>(return_l, member, dst), 0)...}; (void)_;
			return return_l; // We can't serialize into fake result member
        });
}

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
    const Line *ptr = e.get<Line>();

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
