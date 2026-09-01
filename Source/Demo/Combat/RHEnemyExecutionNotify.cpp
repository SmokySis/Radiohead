#include "RHEnemyExecutionNotify.h"

#include "Components/SkeletalMeshComponent.h"
#include "Demo/AI/RHEnemyAIComponent.h"

void URHEnemyExecutionNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (MeshComp && MeshComp->GetOwner())
	{
		if (URHEnemyAIComponent* AI = MeshComp->GetOwner()->FindComponentByClass<URHEnemyAIComponent>())
		{
			AI->ApplyExecutionDamage();
		}
	}
}

FString URHEnemyExecutionNotify::GetNotifyName_Implementation() const
{
	return TEXT("Enemy Execution Damage");
}
