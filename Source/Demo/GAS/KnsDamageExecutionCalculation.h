#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "KnsDamageExecutionCalculation.generated.h"

UCLASS()
class DEMO_API UKnsDamageExecutionCalculation : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	UKnsDamageExecutionCalculation();

	static float CalculateDamage(
		float AttackPower,
		float Resistance,
		float Defense,
		float DamageReduction,
		float MoveMultiplier,
		float CritMultiplier,
		bool bIsCriticalHit);

	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;

protected:
	DECLARE_ATTRIBUTE_CAPTUREDEF(AttackPower);
	DECLARE_ATTRIBUTE_CAPTUREDEF(CritRate);
	DECLARE_ATTRIBUTE_CAPTUREDEF(Resistance);
	DECLARE_ATTRIBUTE_CAPTUREDEF(Defense);
	DECLARE_ATTRIBUTE_CAPTUREDEF(DamageReduction);
};
