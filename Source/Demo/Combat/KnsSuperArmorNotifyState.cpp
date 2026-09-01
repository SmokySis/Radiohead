#include "KnsSuperArmorNotifyState.h"

#include "Components/SkeletalMeshComponent.h"
#include "Demo/Combo/KnsComboComponent.h"

void UKnsSuperArmorNotifyState::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (MeshComp && MeshComp->GetOwner())
	{
		if (UKnsComboComponent* ComboComponent = MeshComp->GetOwner()->FindComponentByClass<UKnsComboComponent>())
		{
			ComboComponent->SetSuperArmor(true);
		}
	}
}

void UKnsSuperArmorNotifyState::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (MeshComp && MeshComp->GetOwner())
	{
		if (UKnsComboComponent* ComboComponent = MeshComp->GetOwner()->FindComponentByClass<UKnsComboComponent>())
		{
			ComboComponent->SetSuperArmor(false);
		}
	}
}

FString UKnsSuperArmorNotifyState::GetNotifyName_Implementation() const
{
	return TEXT("Kns Super Armor");
}
