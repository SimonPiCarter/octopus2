#include "StepContainer.hh"

namespace octopus
{

StepManager<HitPointStep, HitPointMaxStep, PositionStep, AttackWindupStep, AttackReloadStep> makeDefaultStepManager()
{
	return StepManager<HitPointStep, HitPointMaxStep, PositionStep, AttackWindupStep, AttackReloadStep>();
}

} // octopus
