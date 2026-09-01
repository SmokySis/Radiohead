#include "KnsHealGameplayEffect.h"

#include "KnsCommonAttributeSet.h"

UKnsHealGameplayEffect::UKnsHealGameplayEffect()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayModifierInfo HealthModifier;
	HealthModifier.Attribute = UKnsCommonAttributeSet::GetHealthAttribute();
	FSetByCallerFloat HealthSetByCaller;
	HealthSetByCaller.DataName = TEXT("Resource.Health");
	HealthModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(HealthSetByCaller);
	Modifiers.Add(HealthModifier);
}
