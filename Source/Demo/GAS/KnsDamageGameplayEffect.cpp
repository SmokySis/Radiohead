#include "KnsDamageGameplayEffect.h"

#include "KnsDamageExecutionCalculation.h"

UKnsDamageGameplayEffect::UKnsDamageGameplayEffect()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayEffectExecutionDefinition DamageExecution;
	DamageExecution.CalculationClass = UKnsDamageExecutionCalculation::StaticClass();
	Executions.Add(DamageExecution);
}
