#pragma once

#include "flecs.h"
#include <variant>

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
