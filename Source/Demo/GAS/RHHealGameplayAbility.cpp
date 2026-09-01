#include "RHHealGameplayAbility.h"

#include "AbilitySystemComponent.h"
#include "KnsCommonAttributeSet.h"
#include "KnsHealGameplayEffect.h"

URHHealGameplayAbility::URHHealGameplayAbility()
{
	HealEffectClass = UKnsHealGameplayEffect::StaticClass();
}

void URHHealGameplayAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	if (!ASC || !HealEffectClass)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	Context.AddSourceObject(ActorInfo->AvatarActor.Get());

	FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(HealEffectClass, 1.f, Context);
	if (!SpecHandle.IsValid())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	float AppliedHeal = FMath::Max(HealAmount, 0.f);
	if (const UKnsCommonAttributeSet* CommonAS = ASC->GetSet<UKnsCommonAttributeSet>())
	{
		AppliedHeal = FMath::Min(AppliedHeal, FMath::Max(CommonAS->GetMaxHealth() - CommonAS->GetHealth(), 0.f));
	}

	SpecHandle.Data->SetSetByCallerMagnitude(TEXT("Resource.Health"), AppliedHeal);
	ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());

	EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
}
