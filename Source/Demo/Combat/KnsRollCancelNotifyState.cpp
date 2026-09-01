#include "KnsRollCancelNotifyState.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Components/SkeletalMeshComponent.h"

void UKnsRollCancelNotifyState::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (MeshComp && MeshComp->GetOwner())
	{
		if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(MeshComp->GetOwner()))
		{
			if (UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent())
			{
				ASC->AddLooseGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("Combo.Cancel.Roll"), false));
			}
		}
	}
}

void UKnsRollCancelNotifyState::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (MeshComp && MeshComp->GetOwner())
	{
		if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(MeshComp->GetOwner()))
		{
			if (UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent())
			{
				ASC->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("Combo.Cancel.Roll"), false));
			}
		}
	}
}

FString UKnsRollCancelNotifyState::GetNotifyName_Implementation() const
{
	return TEXT("Kns Roll Cancel");
}
