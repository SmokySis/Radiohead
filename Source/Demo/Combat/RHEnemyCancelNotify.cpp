#include "RHEnemyCancelNotify.h"

#include "Components/SkeletalMeshComponent.h"
#include "Demo/AI/RHEnemyAIComponent.h"

void URHEnemyCancelNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (MeshComp && MeshComp->GetOwner())
	{
		if (URHEnemyAIComponent* AI = MeshComp->GetOwner()->FindComponentByClass<URHEnemyAIComponent>())
		{
			AI->SendAIEvent(FGameplayTag::RequestGameplayTag(TEXT("AI.Enemy.Cancel"), false));
		}
	}
}

FString URHEnemyCancelNotify::GetNotifyName_Implementation() const
{
	return TEXT("Enemy Cancel");
}
