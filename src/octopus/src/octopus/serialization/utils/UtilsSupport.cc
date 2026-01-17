#include "UtilsSupport.hh"

#include <string>

#include "octopus/components/basic/hitpoint/HitPoint.hh"
#include "octopus/components/basic/hitpoint/HitPointMax.hh"
#include "octopus/components/basic/position/Position.hh"
#include "octopus/components/advanced/buff/DebuffAll.hh"

namespace octopus
{

void utils_support(flecs::world& ecs)
{
    ecs.component<Fixed>()
        .member("data", &Fixed::_data);

    ecs.component<Vector>()
        .member("x", &Vector::x)
        .member("y", &Vector::y);

    // Register reflection for std::string
    ecs.component<std::string>()
        .opaque(flecs::String) // Opaque type that maps to string
            .serialize([](const flecs::serializer *s, const std::string *data) {
                const char *str = data->c_str();
                return s->value(flecs::String, &str); // Forward to serializer
            })
            .assign_string([](std::string* data, const char *value) {
                *data = value; // Assign new value to std::string
            });
}

std::string save_world(flecs::world &ecs) {
    std::string json;

	flecs::query<> q = ecs.query_builder()
		.without<flecs::Identifier>(flecs::Symbol)
            .term_at(0).parent()
		.without<flecs::Identifier>(flecs::Symbol)
		.without(flecs::Query)
		.without(flecs::Observer)
		.without(flecs::Private)
		.without(flecs::Module)
		.without(flecs::Prefab)
		.without(flecs::Disabled)
		.without(flecs::Empty)
		.without(flecs::Monitor)
		.without(flecs::System)
		.without(flecs::Pipeline)
		.without(flecs::Phase)
		.without(flecs::Final)
		.without<flecs::Component>()
		.without<flecs::Member>()
		.without<flecs::doc::Description>(flecs::doc::Brief)
		.build()
	;

	ecs_iter_to_json_desc_t desc = ECS_ITER_TO_JSON_INIT;
	desc.serialize_table = true;
	json = q.iter().to_json(&desc);
    return json;
}

void load_world(flecs::world &ecs, std::string const &json) {
    // load saved data
    ecs.from_json(json.c_str());

    // Before saving, emit debuff all event to remove all buffs to avoid double application on load
    emit_debuff_all_event(ecs);
}

} // namespace octopus
