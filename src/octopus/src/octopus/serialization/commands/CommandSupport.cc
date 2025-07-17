#include "CommandSupport.hh"

#include "octopus/commands/basic/move/MoveCommand.hh"
#include "octopus/commands/basic/move/AttackCommand.hh"
#include "octopus/commands/basic/ability/CastCommand.hh"
#include "octopus/commands/basic/rally_point/SetRallyPointCommand.hh"
#include "octopus/commands/basic/NoOpCommand.hh"

#include "octopus/serialization/utils/UtilsSupport.hh"


namespace octopus
{

void basic_commands_support(flecs::world& ecs)
{
    ecs.component<NoOpCommand>();
    ecs.component<MoveCommand>()
        .member("target", &MoveCommand::target)
        .member("flock_handle", &MoveCommand::flock_handle)
        .member("extra_tolerance", &MoveCommand::extra_tolerance);
    ecs.component<AttackCommand>()
        .member("target", &AttackCommand::target)
        .member("target_pos", &AttackCommand::target_pos)
        .member("move", &AttackCommand::move)
        .member("init", &AttackCommand::init)
        .member("flock_handle", &AttackCommand::flock_handle)
        .member("source_pos", &AttackCommand::source_pos)
        .member("patrol", &AttackCommand::patrol)
    ;
    ecs.component<CastCommand>()
        .member("ability", &CastCommand::ability)
        .member("entity_target", &CastCommand::entity_target)
        .member("point_target", &CastCommand::point_target)
    ;
    ecs.component<SetRallyPointCommand>()
        .member("rally_point", &SetRallyPointCommand::rally_point)
    ;

    // After command components to avoid memory corruption (? test start failing randomly)
    ecs.component<NoOpCommand::State>();
    ecs.component<MoveCommand::State>();
    ecs.component<AttackCommand::State>();
    ecs.component<CastCommand::State>();
    ecs.component<SetRallyPointCommand::State>();
}

} // namespace octopus
