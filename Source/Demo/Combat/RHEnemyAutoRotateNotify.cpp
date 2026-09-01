#include "RHEnemyAutoRotateNotify.h"

#include "Components/SkeletalMeshComponent.h"
#include "Demo/AI/RHEnemyAIComponent.h"

void URHEnemyAutoRotateNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (MeshComp && MeshComp->GetOwner())
	{
		if (URHEnemyAIComponent* AI = MeshComp->GetOwner()->FindComponentByClass<URHEnemyAIComponent>())
		{
			AI->SetRotateToPlayer(bOpen);
		}
	}
}
