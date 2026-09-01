#include "RHGuardWindowNotifyState.h"

#include "Components/SkeletalMeshComponent.h"
#include "Demo/Combat/KnsCombatComponent.h"

void URHGuardWindowNotifyState::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (MeshComp && MeshComp->GetOwner())
	{
		if (UKnsCombatComponent* Combat = MeshComp->GetOwner()->FindComponentByClass<UKnsCombatComponent>())
		{
			Combat->SetGuarding(true);
		}
	}
}

void URHGuardWindowNotifyState::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (MeshComp && MeshComp->GetOwner())
	{
		if (UKnsCombatComponent* Combat = MeshComp->GetOwner()->FindComponentByClass<UKnsCombatComponent>())
		{
			Combat->SetGuarding(false);
		}
	}
}
