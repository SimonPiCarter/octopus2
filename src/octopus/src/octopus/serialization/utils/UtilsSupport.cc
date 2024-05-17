#include "UtilsSupport.hh"

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
}

} // namespace octopus