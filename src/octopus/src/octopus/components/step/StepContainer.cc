#include "StepContainer.hh"

namespace octopus
{

StepManager<HitPointStep, HitPointMaxStep, PositionStep, AttackWindupStep, AttackReloadStep, AttackCommandStep> makeDefaultStepManager()
{
	return StepManager<HitPointStep, HitPointMaxStep, PositionStep, AttackWindupStep, AttackReloadStep, AttackCommandStep>();
}

} // octopus
