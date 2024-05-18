#include "CommandSupport.hh"

#include "octopus/commands/basic/move/MoveCommand.hh"

#include "octopus/serialization/utils/UtilsSupport.hh"


namespace octopus
{

void basic_commands_support(flecs::world& ecs)
{
    ecs.component<MoveCommand>()
        .member("target", &MoveCommand::target);
}

} // namespace octopus
