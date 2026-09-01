#include "RHWeaponDefinition.h"

URHWeaponDefinition::URHWeaponDefinition()
{
	AttackHitRule.bEnabled = true;
	AttackHitRule.Mode = ERHOnomRuleMode::Add;
	AttackHitRule.Type = ERHOnomValue::Positive;
	AttackHitRule.Count = 1;

	NormalGuardHitRule.bEnabled = true;
	NormalGuardHitRule.Mode = ERHOnomRuleMode::Add;
	NormalGuardHitRule.Type = ERHOnomValue::Broken;
	NormalGuardHitRule.Count = 1;

	PerfectGuardHitRule.bEnabled = true;
	PerfectGuardHitRule.Mode = ERHOnomRuleMode::Add;
	PerfectGuardHitRule.Type = ERHOnomValue::Negative;
	PerfectGuardHitRule.Count = 1;
}
