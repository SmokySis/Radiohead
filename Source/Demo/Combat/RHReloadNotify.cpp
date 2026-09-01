#include "RHReloadNotify.h"

#include "Components/SkeletalMeshComponent.h"
#include "Demo/Combat/RHCombatComponent.h"

void URHReloadNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (MeshComp && MeshComp->GetOwner())
	{
		if (URHCombatComponent* Combat = MeshComp->GetOwner()->FindComponentByClass<URHCombatComponent>())
		{
			Combat->ExecuteReload(ConsumeResonanceSeconds);
		}
	}
}
