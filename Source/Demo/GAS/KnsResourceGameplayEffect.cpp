#include "KnsResourceGameplayEffect.h"

#include "KnsCommonAttributeSet.h"
#include "KnsPlayerAttributeSet.h"

UKnsResourceGameplayEffect::UKnsResourceGameplayEffect()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayModifierInfo StaminaModifier;
	StaminaModifier.Attribute = UKnsCommonAttributeSet::GetStaminaAttribute();
	FSetByCallerFloat StaminaSetByCaller;
	StaminaSetByCaller.DataName = TEXT("Resource.Stamina");
	StaminaModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(StaminaSetByCaller);
	Modifiers.Add(StaminaModifier);

	FGameplayModifierInfo OnomModifier;
	OnomModifier.Attribute = UKnsPlayerAttributeSet::GetOnomAttribute();
	FSetByCallerFloat OnomSetByCaller;
	OnomSetByCaller.DataName = TEXT("Resource.Onom");
	OnomModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(OnomSetByCaller);
	Modifiers.Add(OnomModifier);

	FGameplayModifierInfo FocusModifier;
	FocusModifier.Attribute = UKnsPlayerAttributeSet::GetFocusAttribute();
	FSetByCallerFloat FocusSetByCaller;
	FocusSetByCaller.DataName = TEXT("Resource.Focus");
	FocusModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FocusSetByCaller);
	Modifiers.Add(FocusModifier);

	FGameplayModifierInfo PoiseModifier;
	PoiseModifier.Attribute = UKnsCommonAttributeSet::GetPoiseAttribute();
	FSetByCallerFloat PoiseSetByCaller;
	PoiseSetByCaller.DataName = TEXT("Resource.Poise");
	PoiseModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(PoiseSetByCaller);
	Modifiers.Add(PoiseModifier);

	FGameplayModifierInfo ResonanceModifier;
	ResonanceModifier.Attribute = UKnsCommonAttributeSet::GetResonanceAttribute();
	FSetByCallerFloat ResonanceSetByCaller;
	ResonanceSetByCaller.DataName = TEXT("Resource.Resonance");
	ResonanceModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(ResonanceSetByCaller);
	Modifiers.Add(ResonanceModifier);
}
