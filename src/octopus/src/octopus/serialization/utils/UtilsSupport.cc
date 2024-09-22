#include "UtilsSupport.hh"

#include <string>

#include "octopus/components/basic/hitpoint/HitPoint.hh"
#include "octopus/components/basic/hitpoint/HitPointMax.hh"
#include "octopus/components/basic/position/Position.hh"

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

} // namespace octopus