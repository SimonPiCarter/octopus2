#include "CommandSupport.hh"

#include "octopus/commands/basic/move/MoveCommand.hh"
#include "octopus/commands/basic/move/AttackCommand.hh"
#include "octopus/commands/basic/ability/CastCommand.hh"

#include "octopus/serialization/utils/UtilsSupport.hh"


namespace octopus
{

void basic_commands_support(flecs::world& ecs)
{
    ecs.component<MoveCommand>()
        .member("target", &MoveCommand::target);
    ecs.component<AttackCommand>()
        .member("target", &AttackCommand::target)
        .member("target_pos", &AttackCommand::target_pos)
        .member("move", &AttackCommand::move);
    ecs.component<CastCommand>()
        .member("ability", &CastCommand::ability)
        .member("entity_target", &CastCommand::entity_target)
        .member("point_target", &CastCommand::point_target);
}

} // namespace octopus
