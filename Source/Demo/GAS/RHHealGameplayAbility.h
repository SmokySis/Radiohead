#pragma once

#include "CoreMinimal.h"
#include "Demo/GAS/RHGameplayAbility.h"
#include "GameplayEffect.h"
#include "RHHealGameplayAbility.generated.h"

UCLASS()
class DEMO_API URHHealGameplayAbility : public URHGameplayAbility
{
	GENERATED_BODY()

public:
	URHHealGameplayAbility();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Heal", meta = (ClampMin = "0"))
	float HealAmount = 50.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Heal")
	TSubclassOf<UGameplayEffect> HealEffectClass;
};
