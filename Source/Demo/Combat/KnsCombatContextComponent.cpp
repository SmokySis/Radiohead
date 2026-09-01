#include "KnsCombatContextComponent.h"

UKnsCombatContextComponent::UKnsCombatContextComponent()
{
}

void UKnsCombatContextComponent::RequestCancelCombo()
{
	OnCancelRequested.Broadcast();
}

void UKnsCombatContextComponent::RequestHitConditionTag(FGameplayTag Tag)
{
	if (Tag.IsValid())
	{
		OnHitConditionTagRequested.Broadcast(Tag);
	}
}

void UKnsCombatContextComponent::SetMoveState(UKnsMoveDefinition* Move, FName NodeId, int32 BasePoise, int32 EffectivePoise, bool bActive, float InResistance)
{
	CurrentMove = Move;
	CurrentNodeId = NodeId;
	BasePoiseLevel = BasePoise;
	EffectivePoiseLevel = EffectivePoise;
	Resistance = FMath::Clamp(InResistance, 0.f, 1.f);
	bComboActive = bActive;

	OnMoveChanged.Broadcast(CurrentMove, CurrentNodeId);
	OnComboActiveChanged.Broadcast(bComboActive);
	OnPoiseChanged.Broadcast(BasePoiseLevel, EffectivePoiseLevel);
}

void UKnsCombatContextComponent::SetPoiseState(int32 BasePoise, int32 EffectivePoise, float InResistance)
{
	BasePoiseLevel = BasePoise;
	EffectivePoiseLevel = EffectivePoise;
	Resistance = FMath::Clamp(InResistance, 0.f, 1.f);
	OnPoiseChanged.Broadcast(BasePoiseLevel, EffectivePoiseLevel);
}

void UKnsCombatContextComponent::ClearCombatState()
{
	CurrentMove = nullptr;
	CurrentNodeId = NAME_None;
	BasePoiseLevel = 0;
	EffectivePoiseLevel = 0;
	Resistance = 0.f;
	bComboActive = false;

	OnMoveChanged.Broadcast(nullptr, NAME_None);
	OnComboActiveChanged.Broadcast(false);
	OnPoiseChanged.Broadcast(0, 0);
}

UKnsMoveDefinition* UKnsCombatContextComponent::GetCurrentMove() const
{
	return CurrentMove;
}

int32 UKnsCombatContextComponent::GetEffectivePoiseLevel() const
{
	return EffectivePoiseLevel;
}

float UKnsCombatContextComponent::GetResistance() const
{
	return Resistance;
}

bool UKnsCombatContextComponent::IsComboActive() const
{
	return bComboActive;
}
