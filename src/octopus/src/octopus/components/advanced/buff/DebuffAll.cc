#include "DebuffAll.hh"
#include "octopus/world/player/PlayerInfo.hh"

namespace octopus
{

flecs::entity get_debuff_all_entity(flecs::world &ecs) {
	return ecs.entity("DebufAll");
}

void emit_debuff_all_event(flecs::world &ecs) {
	ecs.event<DebuffAll>()
		.id<DebuffAll>()
		.entity(get_debuff_all_entity(ecs))
		.emit();

	// reset all buffs to avoid double player buff
	ecs.query<PlayerInfo>()
		.each([&ecs](flecs::entity player, PlayerInfo const &)
	{
		ecs.event<DebuffAll>()
			.id<PlayerInfo>()
			.entity(player)
			.emit();
	});
}

}
