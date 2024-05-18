#include "CommandSupport.hh"

#include "octopus/commands/basic/move/MoveCommand.hh"
#include "octopus/commands/basic/move/AttackCommand.hh"

#include "octopus/serialization/utils/UtilsSupport.hh"


namespace octopus
{

void basic_commands_support(flecs::world& ecs)
{
    ecs.component<MoveCommand>()
        .member("target", &MoveCommand::target);
    ecs.component<AttackCommand>()
        .member("target", &AttackCommand::target);
}

} // namespace octopus
