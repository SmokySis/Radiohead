#include "RHParryWindowNotifyState.h"

#include "Components/SkeletalMeshComponent.h"
#include "Demo/Combat/KnsCombatComponent.h"

void URHParryWindowNotifyState::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (MeshComp && MeshComp->GetOwner())
	{
		if (UKnsCombatComponent* Combat = MeshComp->GetOwner()->FindComponentByClass<UKnsCombatComponent>())
		{
			const float Seconds = WindowSeconds > 0.f ? WindowSeconds : TotalDuration;
			Combat->OpenPerfectGuardWindow(Seconds);
		}
	}
}

void URHParryWindowNotifyState::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (MeshComp && MeshComp->GetOwner())
	{
		if (UKnsCombatComponent* Combat = MeshComp->GetOwner()->FindComponentByClass<UKnsCombatComponent>())
		{
			Combat->ClosePerfectGuardWindow();
		}
	}
}
