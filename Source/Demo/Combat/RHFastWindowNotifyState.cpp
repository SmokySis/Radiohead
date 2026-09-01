#include "RHFastWindowNotifyState.h"

#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameplayTagContainer.h"

namespace
{
	UAbilitySystemComponent* GetOwnerASC(USkeletalMeshComponent* MeshComp)
	{
		if (MeshComp && MeshComp->GetOwner())
		{
			if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(MeshComp->GetOwner()))
			{
				return ASI->GetAbilitySystemComponent();
			}
		}
		return nullptr;
	}
}

void URHFastWindowNotifyState::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (UAbilitySystemComponent* ASC = GetOwnerASC(MeshComp))
	{
		const FGameplayTag FastTag = FGameplayTag::RequestGameplayTag(TEXT("Window.AllowFast"), false);
		if (FastTag.IsValid())
		{
			ASC->AddLooseGameplayTag(FastTag);
		}
	}
}

void URHFastWindowNotifyState::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (UAbilitySystemComponent* ASC = GetOwnerASC(MeshComp))
	{
		const FGameplayTag FastTag = FGameplayTag::RequestGameplayTag(TEXT("Window.AllowFast"), false);
		if (FastTag.IsValid())
		{
			ASC->RemoveLooseGameplayTag(FastTag);
		}
	}
}
