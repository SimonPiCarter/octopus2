#pragma once

#include "octopus/components/advanced/buff/DebuffAll.hh"

namespace octopus
{

template <typename Buff, typename Component, typename Applier, typename Reverter>
void declare_stats_buff_systems(flecs::world &ecs, Applier apply, Reverter revert, bool add_debuff_all_system = true) {
	ecs.observer<Buff const, Component>()
		.event(flecs::OnSet)
		.each([apply](flecs::entity e, Buff const& buff, Component &comp) {
			apply(buff, comp);
		});

	ecs.observer<Buff const, Component>()
		.event(flecs::OnRemove)
		.each([revert](flecs::entity e, Buff const& buff, Component &comp) {
			revert(buff, comp);
		});

	flecs::query query_units = ecs.query_builder<Buff const, Component>()
		.build();

	if(add_debuff_all_system)
	{
		get_debuff_all_entity(ecs).add<DebuffAll>();
		ecs.observer<DebuffAll const>()
			.template event<DebuffAll>()
			.each([query_units, revert] (flecs::entity, DebuffAll const &) {
				query_units.each([revert](flecs::entity e, Buff const &buff, Component &comp)
				{
					revert(buff, comp);
				});
			});
	}
}

}
