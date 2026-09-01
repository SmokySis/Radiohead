#include "RHReverseJustReloadWindowNotifyState.h"

#include "Components/SkeletalMeshComponent.h"
#include "Demo/Combat/RHCombatComponent.h"

void URHReverseJustReloadWindowNotifyState::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (MeshComp && MeshComp->GetOwner())
	{
		if (URHCombatComponent* Combat = MeshComp->GetOwner()->FindComponentByClass<URHCombatComponent>())
		{
			Combat->OpenReverseJustReloadWindow(ConsumeResonanceSeconds);
		}
	}
}

void URHReverseJustReloadWindowNotifyState::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (MeshComp && MeshComp->GetOwner())
	{
		if (URHCombatComponent* Combat = MeshComp->GetOwner()->FindComponentByClass<URHCombatComponent>())
		{
			Combat->CloseReverseJustReloadWindow();
		}
	}
}
