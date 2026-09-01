#include "RHCombatWindowNotifyState.h"

#include "Components/SkeletalMeshComponent.h"
#include "Demo/Combat/RHCombatActionInterface.h"

void URHCombatWindowNotifyState::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (MeshComp && MeshComp->GetOwner())
	{
		if (IRHCombatActionInterface* ActionInterface = Cast<IRHCombatActionInterface>(MeshComp->GetOwner()))
		{
			ActionInterface->SetComboWindowOpen(true);
		}
	}
}

void URHCombatWindowNotifyState::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (MeshComp && MeshComp->GetOwner())
	{
		if (IRHCombatActionInterface* ActionInterface = Cast<IRHCombatActionInterface>(MeshComp->GetOwner()))
		{
			ActionInterface->SetComboWindowOpen(false);
		}
	}
}
