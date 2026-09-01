#include "RHJustLoadNotify.h"

#include "Components/SkeletalMeshComponent.h"
#include "Demo/Combat/RHCombatComponent.h"

void URHJustLoadNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (MeshComp && MeshComp->GetOwner())
	{
		if (URHCombatComponent* Combat = MeshComp->GetOwner()->FindComponentByClass<URHCombatComponent>())
		{
			Combat->ExecuteJustLoad(bPlayVFX);
		}
	}
}
