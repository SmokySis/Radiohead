#include "RHBlitzWindowNotifyState.h"

#include "Components/SkeletalMeshComponent.h"
#include "Demo/Combat/RHCombatComponent.h"

void URHBlitzWindowNotifyState::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (MeshComp && MeshComp->GetOwner())
	{
		if (URHCombatComponent* Combat = MeshComp->GetOwner()->FindComponentByClass<URHCombatComponent>())
		{
			Combat->SetBlitzWindow(true);
			Combat->BeginHitbox(HitboxTag);
		}
	}
}

void URHBlitzWindowNotifyState::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (MeshComp && MeshComp->GetOwner())
	{
		if (URHCombatComponent* Combat = MeshComp->GetOwner()->FindComponentByClass<URHCombatComponent>())
		{
			Combat->SetBlitzWindow(false);
			Combat->EndHitbox(HitboxTag);
		}
	}
}
