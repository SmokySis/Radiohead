#include "KnsTouchConditionNotifyState.h"

#include "Components/SkeletalMeshComponent.h"
#include "Demo/Combat/KnsCombatComponent.h"

void UKnsTouchConditionNotifyState::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (MeshComp && MeshComp->GetOwner())
	{
		if (UKnsCombatComponent* CombatComponent = MeshComp->GetOwner()->FindComponentByClass<UKnsCombatComponent>())
		{
			CombatComponent->SetTouchConditionEnabled(true);
		}
	}
}

void UKnsTouchConditionNotifyState::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (MeshComp && MeshComp->GetOwner())
	{
		if (UKnsCombatComponent* CombatComponent = MeshComp->GetOwner()->FindComponentByClass<UKnsCombatComponent>())
		{
			CombatComponent->SetTouchConditionEnabled(false);
		}
	}
}

FString UKnsTouchConditionNotifyState::GetNotifyName_Implementation() const
{
	return TEXT("Kns Touch Condition");
}
