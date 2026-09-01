#include "KnsHitboxNotifyState.h"

#include "Components/SkeletalMeshComponent.h"
#include "Demo/Combat/KnsCombatComponent.h"

void UKnsHitboxNotifyState::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (MeshComp && MeshComp->GetOwner())
	{
		if (UKnsCombatComponent* CombatComponent = MeshComp->GetOwner()->FindComponentByClass<UKnsCombatComponent>())
		{
			CombatComponent->BeginHitbox(HitboxTag);
		}
	}
}

void UKnsHitboxNotifyState::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (MeshComp && MeshComp->GetOwner())
	{
		if (UKnsCombatComponent* CombatComponent = MeshComp->GetOwner()->FindComponentByClass<UKnsCombatComponent>())
		{
			CombatComponent->EndHitbox(HitboxTag);
		}
	}
}

FString UKnsHitboxNotifyState::GetNotifyName_Implementation() const
{
	return TEXT("Kns Hitbox");
}
