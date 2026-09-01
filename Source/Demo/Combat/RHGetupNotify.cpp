#include "RHGetupNotify.h"

#include "Components/SkeletalMeshComponent.h"
#include "Demo/Combat/KnsCombatComponent.h"

void URHGetupNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (MeshComp && MeshComp->GetOwner())
	{
		if (UKnsCombatComponent* Combat = MeshComp->GetOwner()->FindComponentByClass<UKnsCombatComponent>())
		{
			Combat->EndKnockdown();
		}
	}
}
