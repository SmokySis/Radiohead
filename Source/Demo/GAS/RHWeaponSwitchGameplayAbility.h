#pragma once

#include "CoreMinimal.h"
#include "Demo/GAS/RHGameplayAbility.h"
#include "RHWeaponSwitchGameplayAbility.generated.h"

class URHWeaponDefinition;

UCLASS()
class DEMO_API URHWeaponSwitchGameplayAbility : public URHGameplayAbility
{
	GENERATED_BODY()

public:
	URHWeaponSwitchGameplayAbility();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	/** 临时武器数据 DA（武器类/主手 Socket 从 DA 取）。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<URHWeaponDefinition> WeaponDefinition;
};
