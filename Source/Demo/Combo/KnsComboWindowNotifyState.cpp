#include "KnsComboWindowNotifyState.h"

#include "Components/SkeletalMeshComponent.h"
#include "Demo/AI/RHEnemyCombatComponent.h"
#include "KnsComboComponent.h"

void UKnsComboWindowNotifyState::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (MeshComp && MeshComp->GetOwner())
	{
		if (URHEnemyCombatComponent* EnemyCombat = MeshComp->GetOwner()->FindComponentByClass<URHEnemyCombatComponent>())
		{
			EnemyCombat->SetComboWindowOpen(true);
		}
		else if (UKnsComboComponent* ComboComponent = MeshComp->GetOwner()->FindComponentByClass<UKnsComboComponent>())
		{
			ComboComponent->SetComboWindowOpen(true);
		}
	}
}

void UKnsComboWindowNotifyState::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (MeshComp && MeshComp->GetOwner())
	{
		if (URHEnemyCombatComponent* EnemyCombat = MeshComp->GetOwner()->FindComponentByClass<URHEnemyCombatComponent>())
		{
			EnemyCombat->SetComboWindowOpen(false);
		}
		else if (UKnsComboComponent* ComboComponent = MeshComp->GetOwner()->FindComponentByClass<UKnsComboComponent>())
		{
			ComboComponent->SetComboWindowOpen(false);
		}
	}
}

FString UKnsComboWindowNotifyState::GetNotifyName_Implementation() const
{
	return TEXT("Kns Combo Window");
}
