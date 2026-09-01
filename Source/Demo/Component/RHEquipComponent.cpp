#include "RHEquipComponent.h"

#include "AbilitySystemInterface.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Demo/Character/BaseCharacter.h"
#include "Demo/Combat/KnsCombatComponent.h"
#include "Demo/Combat/RHCombatComponent.h"
#include "Demo/GAS/KnsAbilitySystemComponent.h"
#include "Demo/Onom/RHCoreDefinition.h"
#include "Demo/Onom/RHOnomComponent.h"
#include "GameplayTagContainer.h"

URHEquipComponent::URHEquipComponent()
{
}

void URHEquipComponent::BeginPlay()
{
	Super::BeginPlay();

	ApplyOnomCore();
	GrantRhythmWeaponAbilities();
	if (!WeaponSlots.IsEmpty())
	{
		ApplyWeaponSlot(0);
	}
}

bool URHEquipComponent::SwitchToWeapon(int32 WeaponIndex)
{
	if (!WeaponSlots.IsValidIndex(WeaponIndex))
	{
		return false;
	}

	if (!IsInFreeState())
	{
		return false;
	}

	// 非 Idle 是靠 Window.Cancel.Special 通过的：先打断当前动作再换武器。
	if (URHCombatComponent* RHCombat = GetOwner()->FindComponentByClass<URHCombatComponent>())
	{
		if (!RHCombat->IsIdle())
		{
			RHCombat->CancelAction();
		}
	}

	ApplyWeaponSlot(WeaponIndex);
	return true;
}

bool URHEquipComponent::SwitchToWeaponWithTactic(int32 WeaponIndex)
{
	if (!WeaponSlots.IsValidIndex(WeaponIndex))
	{
		return false;
	}

	if (!IsInFreeState())
	{
		return false;
	}

	URHCombatComponent* RHCombat = GetOwner()->FindComponentByClass<URHCombatComponent>();
	// 非 Idle 是靠 Window.Cancel.Special 通过的：先打断当前动作，避免“武器已换、招式还在播”。
	if (RHCombat && !RHCombat->IsIdle())
	{
		RHCombat->CancelAction();
	}

	ApplyWeaponSlot(WeaponIndex);
	if (RHCombat)
	{
		RHCombat->TryPlaySwitchTactic();
	}
	return true;
}

void URHEquipComponent::GrantRhythmWeaponAbilities()
{
	if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(GetOwner()))
	{
		if (UKnsAbilitySystemComponent* ASC = Cast<UKnsAbilitySystemComponent>(ASI->GetAbilitySystemComponent()))
		{
			for (const TObjectPtr<URHOnomActionDefinition>& Action : RhythmWeapons)
			{
				if (Action && Action->EffectAbility && !ASC->FindAbilitySpecFromClass(Action->EffectAbility))
				{
					ASC->GiveAbility(FGameplayAbilitySpec(Action->EffectAbility, 1, INDEX_NONE, GetOwner()));
				}
			}
		}
	}
}

void URHEquipComponent::ApplyWeaponSlot(int32 WeaponIndex)
{
	const FRHWeaponSlot& Slot = WeaponSlots[WeaponIndex];

	UKnsCombatComponent* Combat = GetOwner()->FindComponentByClass<UKnsCombatComponent>();
	if (!Combat || !Combat->SwitchWeapon(Slot.WeaponDefinition))
	{
		return;
	}

	if (ABaseCharacter* Character = Cast<ABaseCharacter>(GetOwner()))
	{
		if (USkeletalMeshComponent* Mesh = Character->GetMesh())
		{
			if (CurrentAnimLayerClass)
			{
				Mesh->UnlinkAnimClassLayers(CurrentAnimLayerClass);
			}
			CurrentAnimLayerClass = Slot.AnimLayerClass;
			if (CurrentAnimLayerClass)
			{
				Mesh->LinkAnimClassLayers(CurrentAnimLayerClass);
			}
		}
	}

	CurrentWeaponIndex = WeaponIndex;
	OnWeaponSwitched.Broadcast(WeaponIndex);
}

void URHEquipComponent::ApplyOnomCore()
{
	if (URHOnomComponent* Onom = GetOwner()->FindComponentByClass<URHOnomComponent>())
	{
		Onom->CoreDefinition = OnomCore;
	}
}

void URHEquipComponent::SetOnomCore(URHCoreDefinition* Core)
{
	OnomCore = Core;
	ApplyOnomCore();
}

URHCoreDefinition* URHEquipComponent::GetOnomCore() const
{
	return OnomCore;
}

int32 URHEquipComponent::GetCurrentWeaponIndex() const
{
	return CurrentWeaponIndex;
}

URHWeaponDefinition* URHEquipComponent::GetCurrentWeaponDefinition() const
{
	if (WeaponSlots.IsValidIndex(CurrentWeaponIndex))
	{
		return WeaponSlots[CurrentWeaponIndex].WeaponDefinition;
	}
	return nullptr;
}

bool URHEquipComponent::IsInFreeState() const
{
	if (URHCombatComponent* RHCombat = GetOwner()->FindComponentByClass<URHCombatComponent>())
	{
		if (RHCombat->IsIdle())
		{
			return true;
		}
	}
	return IsSpecialCancelActive();
}

bool URHEquipComponent::IsSpecialCancelActive() const
{
	if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(GetOwner()))
	{
		if (UKnsAbilitySystemComponent* ASC = Cast<UKnsAbilitySystemComponent>(ASI->GetAbilitySystemComponent()))
		{
			const FGameplayTag SpecialTag = FGameplayTag::RequestGameplayTag(TEXT("Window.Cancel.Special"), false);
			return SpecialTag.IsValid() && ASC->HasMatchingGameplayTag(SpecialTag);
		}
	}
	return false;
}
