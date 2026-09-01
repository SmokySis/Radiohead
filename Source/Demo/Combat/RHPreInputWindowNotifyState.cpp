#include "RHPreInputWindowNotifyState.h"

#include "Components/SkeletalMeshComponent.h"
#include "Demo/Combat/RHCombatActionInterface.h"

void URHPreInputWindowNotifyState::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (MeshComp && MeshComp->GetOwner())
	{
		if (IRHCombatActionInterface* ActionInterface = Cast<IRHCombatActionInterface>(MeshComp->GetOwner()))
		{
			ActionInterface->SetPreInputWindowOpen(true);
		}
	}
}

void URHPreInputWindowNotifyState::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (MeshComp && MeshComp->GetOwner())
	{
		if (IRHCombatActionInterface* ActionInterface = Cast<IRHCombatActionInterface>(MeshComp->GetOwner()))
		{
			ActionInterface->SetPreInputWindowOpen(false);
		}
	}
}
