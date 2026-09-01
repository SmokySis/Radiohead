#include "KnsDamageExecutionCalculation.h"

#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "KnsCommonAttributeSet.h"

UKnsDamageExecutionCalculation::UKnsDamageExecutionCalculation()
{
	DEFINE_ATTRIBUTE_CAPTUREDEF(UKnsCommonAttributeSet, AttackPower, Source, true);
	DEFINE_ATTRIBUTE_CAPTUREDEF(UKnsCommonAttributeSet, CritRate, Source, true);
	DEFINE_ATTRIBUTE_CAPTUREDEF(UKnsCommonAttributeSet, Resistance, Target, true);
	DEFINE_ATTRIBUTE_CAPTUREDEF(UKnsCommonAttributeSet, Defense, Target, true);
	DEFINE_ATTRIBUTE_CAPTUREDEF(UKnsCommonAttributeSet, DamageReduction, Target, true);

	RelevantAttributesToCapture.Add(AttackPowerDef);
	RelevantAttributesToCapture.Add(CritRateDef);
	RelevantAttributesToCapture.Add(ResistanceDef);
	RelevantAttributesToCapture.Add(DefenseDef);
	RelevantAttributesToCapture.Add(DamageReductionDef);
}

float UKnsDamageExecutionCalculation::CalculateDamage(
	float AttackPower,
	float Resistance,
	float Defense,
	float DamageReduction,
	float MoveMultiplier,
	float CritMultiplier,
	bool bIsCriticalHit)
{
	float Damage = AttackPower * MoveMultiplier * FMath::Max(1.f - FMath::Clamp(Resistance, 0.f, 1.f), 0.f);

	if (bIsCriticalHit)
	{
		Damage *= CritMultiplier;
	}

	const float DefenseFactor = 100.f / (100.f + FMath::Max(Defense, 0.f));
	const float ReductionFactor = FMath::Clamp(1.f - DamageReduction, 0.f, 1.f);
	return Damage * DefenseFactor * ReductionFactor;
}

void UKnsDamageExecutionCalculation::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	UAbilitySystemComponent* TargetASC = ExecutionParams.GetTargetAbilitySystemComponent();
	if (!TargetASC)
	{
		return;
	}

	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();

	FAggregatorEvaluateParameters EvaluationParameters;
	EvaluationParameters.SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	EvaluationParameters.TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	float AttackPower = 0.f;
	float CritRate = 0.f;
	float Resistance = 0.f;
	float Defense = 0.f;
	float DamageReduction = 0.f;

	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(AttackPowerDef, EvaluationParameters, AttackPower);
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(CritRateDef, EvaluationParameters, CritRate);
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(ResistanceDef, EvaluationParameters, Resistance);
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DefenseDef, EvaluationParameters, Defense);
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageReductionDef, EvaluationParameters, DamageReduction);

	const float MoveMultiplier = Spec.GetSetByCallerMagnitude(TEXT("Damage.Multiplier"), false, 1.f);
	const float CritMultiplier = Spec.GetSetByCallerMagnitude(TEXT("Damage.CritMultiplier"), false, 1.5f);
	const float FlatDamage = Spec.GetSetByCallerMagnitude(TEXT("Damage.Flat"), false, 0.f);

	const float ClampedCritRate = FMath::Clamp(CritRate, 0.f, 1.f);
	const bool bIsCriticalHit = ClampedCritRate > 0.f && FMath::FRand() <= ClampedCritRate;

	float EffectiveAttack = AttackPower;
	float EffectiveMoveMultiplier = MoveMultiplier;
	bool bEffectiveCritical = bIsCriticalHit;
	if (FlatDamage > 0.f)
	{
		EffectiveAttack = FlatDamage;
		EffectiveMoveMultiplier = 1.f;
		bEffectiveCritical = false;
	}

	const float FinalDamage = CalculateDamage(
		EffectiveAttack,
		Resistance,
		Defense,
		DamageReduction,
		EffectiveMoveMultiplier,
		CritMultiplier,
		bEffectiveCritical);

	if (FinalDamage <= 0.f)
	{
		return;
	}

	OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(
		UKnsCommonAttributeSet::GetHealthAttribute(),
		EGameplayModOp::Additive,
		-FinalDamage));
}
