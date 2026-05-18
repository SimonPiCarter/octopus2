
namespace octopus {

inline bool is_caster(flecs::world const &ecs,
					 flecs::entity e,
					 std::string const &cast_name) {
	if (!e.is_valid()) {
		return false;
	}
	auto caster = e.try_get<octopus::Caster>();
	return caster
		&& e.has<octopus::Caster>(ecs.component(cast_name.c_str()));
}

template<typename StepManager_t>
flecs::entity find_best_entity_for_casting(flecs::world const &ecs,
											AbilityTemplateLibrary<StepManager_t> const &ability_library,
											std::vector<flecs::entity> const &group,
											std::string const &cast_name,
											octopus::Vector const &target_pos,
											InputStatus &status) {
	using namespace octopus;
	octopus::AbilityTemplate<StepManager_t> const &ability = ability_library.get(cast_name);

	bool found_missing_resource = false;
	bool found_cooldown = false;
	status.cooldown_ratio = 1.;
	/// @todo Tri entre les entités :
	/// - Proximité
	/// Filtre :
	/// - Caster qui peut cast la demande
	/// - Pas de cast du même sort dans la queue (sauf si reload = 0) (sauf si aucun autre candidat)
	/// - Resource check (en prenant en compte tous les cast dans la queue si booléen queued = true [add])
	for(flecs::entity const &e : group) {
		if(!is_caster(ecs, e, cast_name)) {
			continue;
		}
		ResourceStock const *stock = e.try_get<ResourceStock>();
		if(!stock) {
			continue;
		}
		Caster const &caster = e.get<Caster>();
		InputStatus current_status = can_cast<StepManager_t>(ecs, *stock, caster, &ability);
		if(current_status.ok) {
			return e;
		}
		for (std::string const & expl : current_status.other_explanations) {
			if (expl == "MISSING_RESOURCES") {
				found_missing_resource = true;
			} else if (expl == "COOLDOWN") {
				found_cooldown = true;
			}
		}
		if (current_status.cooldown_ratio < status.cooldown_ratio) {
			status.cooldown_ratio = current_status.cooldown_ratio;
			status.cooldown_ticks_remaining = current_status.cooldown_ticks_remaining;
		}
	}
	status.ok = false;
	status.other_explanations.push_back("NO_VALID_CANDIDATE");
	return flecs::entity();
}

template<typename StepManager_t>
InputStatus get_input_status(flecs::world &ecs, AbilityTemplateLibrary<StepManager_t> const &ability_lib, InputCast const &input) {
	InputStatus status;
	octopus::AbilityTemplate<StepManager_t> const *ability = ability_lib.try_get(input.cast_command.ability);

	if(!ability) {
		status.ok = false;
		status.other_explanations.push_back("UNREGISTERED_ABILITY");
		return status;
	}

	flecs::entity candidate = find_best_entity_for_casting(ecs, ability_lib, input.candidates, input.cast_command.ability, input.cast_command.point_target, status);
	if (!candidate.is_valid()) {
		return status;
	}
	status.entity = candidate;

	// Get player from candidate
	flecs::entity player = get_player_from_appartenance(candidate, ecs);
	if (!player.is_valid()) {
		status.ok = false;
		const std::string reason = "NO_PLAYER_APPARTENANCE";
		status.other_explanations.push_back(reason);
		return status;
	}

	// Check upgrades requirements
	status.missing_upgrades = explain_unmet_requirements(candidate, ecs, ability->get_requirements());
	status.ok &= status.missing_upgrades.empty();

	return status;
}

template<typename StepManager_t, typename command_variant_t>
void handle_cast(
	InputCast const &input,
	AbilityTemplateLibrary<StepManager_t> const &ability_lib,
	flecs::world &ecs,
	StepManager_t &manager) {
	InputStatus status = octopus::get_input_status(ecs, ability_lib, input);
	if (!status.ok) {
		// std::cout<<"Can't produce "<<input.production<<std::endl;
		// std::cout<<"missing_upgrades"<<std::endl;
		// for(auto str : status.missing_upgrades) {
		// 	std::cout<<str<<std::endl;
		// }
		// std::cout<<"other_explanations"<<std::endl;
		// for(auto str : status.other_explanations) {
		// 	std::cout<<str<<std::endl;
		// }
		Logger::getDebug() << "Can't cast "<<input.cast_command.ability<<std::endl;
		return;
	}

	auto &&command_queue = status.entity.template get_mut<CommandQueue<command_variant_t>>();
	auto &&queue_l = command_queue._queuedActions;
	// Cast enqueue
	if(input.queued) {
		queue_l.push_back(CommandQueueActionAddBack<command_variant_t> {input.cast_command});
	}
	else {
		// replace queue and finish current action
		queue_l.push_back(CommandQueueActionReplace<command_variant_t> {{input.cast_command}});
		queue_l.push_back(CommandQueueActionDone());
	}
}

} // octopus
