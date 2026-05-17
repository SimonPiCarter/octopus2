
namespace octopus {


template<typename StepManager_t>
flecs::entity find_best_entity_for_production(
	flecs::world const &ecs,
	std::vector<flecs::entity> const &entities,
	std::string const &production_name_p,
	InputStatus &status
) {
	using namespace octopus;
	auto &&prod_library = ecs.try_get<ProductionTemplateLibrary<StepManager_t> >();
	if(!prod_library) {
		return flecs::entity();
	}
	ProductionTemplate<StepManager_t> const & prod_template = prod_library->get(production_name_p);

	flecs::entity best_ent;
	int64_t best_end_time = -1;
	bool found_with_production_queue = false;
	bool found_can_produce = false;
	for(flecs::entity const &e : entities) {
		if(!e.is_valid()) {
			continue;
		}
		octopus::ProductionQueue const * prod_queue = e.try_get<octopus::ProductionQueue>();
		bool has_prod_in_queue = prod_queue && e.has<octopus::ProductionQueue>(ecs.component(production_name_p.c_str()));
		bool can_produce = prod_template.can_produce(e, ecs);
		if(has_prod_in_queue && can_produce) {
			int64_t const queue_duration = octopus::get_queue_duration(*prod_library, prod_queue->queue);
			int64_t const start = prod_queue->start_timestamp;
			if(best_end_time < 0 || start + queue_duration < best_end_time) {
				best_end_time = start + queue_duration;
				best_ent = e;
			}
		}
		if (!found_with_production_queue && has_prod_in_queue) {
			// store best entity to get explaination later if no entity can produce the production
			best_ent = e;
			found_with_production_queue = true;
		}
		if (!found_can_produce && can_produce) {
			found_can_produce = true;
		}
	}
	if (!found_with_production_queue) {
		status.ok = false;
		const std::string reason = "NO_PRODUCTION_QUEUE";
		status.other_explanations.push_back(reason);
	} else if (!found_can_produce) {
		status.ok = false;
		const std::string reason = "CAN_NOT_PRODUCE";
		status.other_explanations.push_back(prod_template.get_production_explaination(best_ent, ecs));
		status.other_explanations.push_back(reason);
	} else {
		return best_ent;
	}
	// best_ent may be valid to get explaination of why the production can't be produced,
	// but it can't produce the production right now, so we return an invalid entity to
	// indicate that no entity can produce the production right now
	return flecs::entity();
}

template<typename command_variant_t, typename StepManager_t>
InputStatus Input<command_variant_t, StepManager_t>::get_input_status(WorldContext<StepManager_t> &world, ProductionTemplateLibrary<StepManager_t> const &prod_lib, InputProduction const &input) {
	InputStatus status;

	// Find best chandidate
	flecs::entity candidate = find_best_entity_for_production<StepManager_t>(world.ecs, input.candidates, input.production, status);
	if (!candidate.is_valid()) {
		status.ok = false;
		const std::string reason = "NO_VALID_CANDIDATE";
		status.other_explanations.push_back(reason);
		return status;
	}
	ProductionTemplate<StepManager_t> const * prod = prod_lib.try_get(input.production);
	if (!prod) {
		status.ok = false;
		const std::string reason = "UNREGISTERED_PRODUCTION";
		status.other_explanations.push_back(reason);
		return status;
	}

	// Get player from candidate
	flecs::entity player = get_player_from_appartenance(candidate, world.ecs);
	if (!player.is_valid()) {
		status.ok = false;
		const std::string reason = "NO_PLAYER_APPARTENANCE";
		status.other_explanations.push_back(reason);
		return status;
	}

	// Check upgrades requirements
	status.missing_upgrades = explain_unmet_requirements(candidate, world.ecs, prod->get_requirements());
	status.ok &= status.missing_upgrades.empty();

	// Check player resources
	PlayerInfo const * player_info = player.try_get<PlayerInfo>();
	ResourceStock const * resource_stock = player.try_get<ResourceStock>();
	ReductionLibrary const * reductionibrary = player.try_get<ReductionLibrary>();
	ResourceSpent * resource_spent = player.try_get_mut<ResourceSpent>();
	status.ok &= check_resources(
		resource_stock ? resource_stock->resource : fast_map<std::string, ResourceInfo>{},
		resource_spent ? resource_spent->resources_spent : fast_map<std::string, Fixed>{},
		reductionibrary ? get_required_resources(reductionibrary->reductions[prod->name()], prod->resource_consumption()) : prod->resource_consumption());
	const std::string reason = "MISSING_RESOURCES";
	status.other_explanations.push_back(reason);

	return status;
}

template<typename command_variant_t, typename StepManager_t>
InputStatus Input<command_variant_t, StepManager_t>::get_input_status(WorldContext<StepManager_t> &world, AbilityTemplateLibrary<StepManager_t> const &ability_lib, InputCast const &input) {
	return InputStatus();
}

} // octopus
