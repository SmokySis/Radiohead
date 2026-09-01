#include "KnsMoveCancelNotifyState.h"

#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Demo/Character/BaseCharacter.h"

void UKnsMoveCancelNotifyState::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

	if (!MeshComp)
	{
		return;
	}

	ABaseCharacter* Character = Cast<ABaseCharacter>(MeshComp->GetOwner());
	if (!Character)
	{
		return;
	}

	// IA_Move 有输入就打断当前蒙太奇。
	if (!Character->GetMoveInputValue().IsNearlyZero())
	{
		if (UAnimMontage* Montage = Cast<UAnimMontage>(Animation))
		{
			Character->StopAnimMontage(Montage);
		}
		else
		{
			Character->StopAnimMontage();
		}
	}
}

FString UKnsMoveCancelNotifyState::GetNotifyName_Implementation() const
{
	return TEXT("Kns Move Cancel");
}
