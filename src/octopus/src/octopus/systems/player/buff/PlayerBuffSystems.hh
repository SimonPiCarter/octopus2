#pragma once

#include "flecs.h"

#include "octopus/components/advanced/player/buff/PlayerBuff.hh"
#include "octopus/components/basic/hitpoint/Destroyable.hh"
#include "octopus/world/player/PlayerInfo.hh"

namespace octopus
{

/// @brief Declare PlayerBuff systems from event and declare serialization
/// @tparam TargetType a component tag used to filter on which entities the buff will apply
/// @tparam BuffType the buff type used to get the apply and revert method and to store the buff info
/// @tparam ComponentTypes... the component to be buffed
/// @param ecs
template<typename TargetType, typename BuffType, typename... ComponentTypes>
void declare_player_buff_systems(flecs::world &ecs, bool add_debuff_all_system = true)
{
	ecs.component<PlayerBuff<TargetType, BuffType, ComponentTypes...>>()
		.member("buff", &PlayerBuff<TargetType, BuffType, ComponentTypes...>::buff)
	;

	flecs::query query_player = ecs.query<PlayerInfo const, PlayerBuff<TargetType, BuffType, ComponentTypes...> const>();
	flecs::query query_units = ecs.query_builder<PlayerAppartenance const, ComponentTypes...>()
		.template with<TargetType>()
		.build();

	// buff all units when buff is added
	ecs.observer<PlayerInfo const, PlayerBuff<TargetType, BuffType, ComponentTypes...> const >()
		.event(flecs::OnSet)
		.each([query_units] (PlayerInfo const &player, PlayerBuff<TargetType, BuffType, ComponentTypes...> const &player_buff) {
			query_units.each([&](flecs::entity e, PlayerAppartenance const &player_appartenance, ComponentTypes&... component)
			{
				if(player_appartenance.idx != player.idx)
				{
					return;
				}
				player_buff.buff.apply(e, component ...);
			});
		});

	if(add_debuff_all_system)
	{
		// revert buff all units Debuff event is emited (usually before a save)
		ecs.observer<PlayerInfo const, PlayerBuff<TargetType, BuffType, ComponentTypes...> const >()
			.template event<DebuffAll>()
			.each([query_units] (PlayerInfo const &player, PlayerBuff<TargetType, BuffType, ComponentTypes...> const &player_buff) {
				query_units.each([&](flecs::entity e, PlayerAppartenance const &player_appartenance, ComponentTypes&... component)
				{
					if(player_appartenance.idx != player.idx)
					{
						return;
					}
					player_buff.buff.revert(e, component ...);
				});
			});
	}

	// revert buff all units when buff is removed
	ecs.observer<PlayerInfo const, PlayerBuff<TargetType, BuffType, ComponentTypes...> const >()
		.event(flecs::OnRemove)
		.each([query_units] (PlayerInfo const &player, PlayerBuff<TargetType, BuffType, ComponentTypes...> const &player_buff) {
			query_units.each([&](flecs::entity e, PlayerAppartenance const &player_appartenance, ComponentTypes&... component)
			{
				if(player_appartenance.idx != player.idx)
				{
					return;
				}
				player_buff.buff.revert(e, component ...);
			});
		});

	// buff new unit when created
	ecs.observer<PlayerAppartenance const, TargetType const, ComponentTypes...>()
		.event(flecs::OnSet)
		.each([query_player](flecs::entity e, PlayerAppartenance const &player_appartenance, TargetType const &, ComponentTypes&... component) {
			query_player.each([&]
				(PlayerInfo const &player, PlayerBuff<TargetType, BuffType, ComponentTypes...> const &player_buff)
				{
					if(player_appartenance.idx != player.idx)
					{
						return;
					}
					player_buff.buff.apply(e, component ...);
				}
			);
		});
}

} // namespace octopus
