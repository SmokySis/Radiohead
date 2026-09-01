#include "RHWeaponSwitchGameplayAbility.h"

#include "AbilitySystemComponent.h"
#include "Demo/Combat/RHCombatComponent.h"

URHWeaponSwitchGameplayAbility::URHWeaponSwitchGameplayAbility()
{
}

void URHWeaponSwitchGameplayAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
	if (URHCombatComponent* Combat = Avatar ? Avatar->FindComponentByClass<URHCombatComponent>() : nullptr)
	{
		Combat->EnterTemporaryWeapon(WeaponDefinition);
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
}
