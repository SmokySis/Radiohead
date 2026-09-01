#include "RHPlayerPanelWidget.h"

#include "AbilitySystemInterface.h"
#include "Components/ProgressBar.h"
#include "Demo/GAS/KnsAbilitySystemComponent.h"
#include "Demo/GAS/KnsCommonAttributeSet.h"
#include "Demo/Onom/RHOnomSettings.h"

void URHPlayerPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (OnomSlot0)
	{
		OnomSlot0->SetSlotState(ERHOnomValue::None);
	}
	if (OnomSlot1)
	{
		OnomSlot1->SetSlotState(ERHOnomValue::None);
	}
	if (OnomSlot2)
	{
		OnomSlot2->SetSlotState(ERHOnomValue::None);
	}
	if (ResonanceBar)
	{
		ResonanceBar->SetPercent(0.f);
		ResonanceBar->SetFillColorAndOpacity(NeutralResonanceColor);
	}
	if (ChargeGauge)
	{
		ChargeGauge->SetChargePercent(0.f);
	}
}

void URHPlayerPanelWidget::BindOnomComponent(URHOnomComponent* InOnom)
{
	if (!InOnom || BoundOnomComponent == InOnom)
	{
		return;
	}

	BoundOnomComponent = InOnom;
	InOnom->OnOnomStateChanged.AddDynamic(this, &URHPlayerPanelWidget::HandleOnomStateChanged);
	HandleOnomStateChanged(InOnom->GetOnomState());
}

void URHPlayerPanelWidget::BindHealthSource(AActor* InActor)
{
	if (!InActor)
	{
		return;
	}

	if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(InActor))
	{
		BoundASC = Cast<UKnsAbilitySystemComponent>(ASI->GetAbilitySystemComponent());
	}

	if (!BoundASC)
	{
		return;
	}

	BoundASC->GetGameplayAttributeValueChangeDelegate(UKnsCommonAttributeSet::GetHealthAttribute())
		.AddUObject(this, &URHPlayerPanelWidget::HandleHealthChanged);
	RefreshHealth();
}

void URHPlayerPanelWidget::BindWeaponDefinition(URHWeaponDefinition* Weapon)
{
	BoundWeaponDefinition = Weapon;
	RefreshSkillPreviews();
}

void URHPlayerPanelWidget::HandleOnomStateChanged(const FRHOnomState& State)
{
	if (OnomSlot0)
	{
		OnomSlot0->SetSlotState(State.Slots.IsValidIndex(0) ? State.Slots[0] : ERHOnomValue::None);
	}
	if (OnomSlot1)
	{
		OnomSlot1->SetSlotState(State.Slots.IsValidIndex(1) ? State.Slots[1] : ERHOnomValue::None);
	}
	if (OnomSlot2)
	{
		OnomSlot2->SetSlotState(State.Slots.IsValidIndex(2) ? State.Slots[2] : ERHOnomValue::None);
	}

	float MaxTime = 10.f;
	if (BoundOnomComponent)
	{
		MaxTime = BoundOnomComponent->GetResonanceDecaySecondsForLevel(State.ResonanceLayers);
	}

	if (ResonanceBar)
	{
		const float Ratio = MaxTime > 0.f ? FMath::Clamp(State.ResonanceTimeRemaining / MaxTime, 0.f, 1.f) : 0.f;
		ResonanceBar->SetPercent(Ratio);
		ResonanceBar->SetFillColorAndOpacity(GetResonanceColor(State.ResonanceLayers, State.ResonanceType));
	}

	if (ChargeGauge)
	{
		ChargeGauge->SetChargePercent(State.ChargePercent);
	}

	RefreshSkillPreviews();
}

void URHPlayerPanelWidget::RefreshSkillPreviews()
{
	if (!BoundWeaponDefinition)
	{
		return;
	}

	TObjectPtr<URHOnomSkillPreviewWidget>* Previews[] = { &SkillPreview0, &SkillPreview1, &SkillPreview2 };

	for (int32 i = 0; i < 3; ++i)
	{
		URHOnomSkillPreviewWidget* Preview = *Previews[i];
		if (!Preview)
		{
			continue;
		}

		if (BoundWeaponDefinition->Skills.IsValidIndex(i))
		{
			// 每个技能按自己的 RequiredCount 模拟实际消耗再匹配，UI 可用性与真实释放一致。
			const FRHOnomConsumptionData Simulated = BoundOnomComponent
				? BoundOnomComponent->SimulateConsumeOnom(BoundWeaponDefinition->Skills[i]->RequiredCount)
				: FRHOnomConsumptionData();
			Preview->SetSkill(BoundWeaponDefinition->Skills[i], Simulated);
		}
		else
		{
			Preview->ClearSkill();
		}
	}
}

void URHPlayerPanelWidget::HandleHealthChanged(const FOnAttributeChangeData& Data)
{
	RefreshHealth();
}

FLinearColor URHPlayerPanelWidget::GetResonanceColor(int32 Layers, ERHOnomPolarity Type) const
{
	if (Layers <= 0)
	{
		return NeutralResonanceColor;
	}

	// 等级梯度：Lv.1 = 起始色，Lv.3 = 结束色（大调 浅橙→橙→红，小调 浅蓝→蓝→亮蓝）。
	const float LevelRatio = FMath::Clamp((float)(Layers - 1) / 2.f, 0.f, 1.f);

	FLinearColor Base = NeutralResonanceColor;
	if (Type == ERHOnomPolarity::Major)
	{
		Base = FMath::Lerp(PositiveResonanceStartColor, PositiveResonanceEndColor, LevelRatio);
	}
	else if (Type == ERHOnomPolarity::Minor)
	{
		Base = FMath::Lerp(NegativeResonanceStartColor, NegativeResonanceEndColor, LevelRatio);
	}

	const float Boost = 1.f + LayerBrightenPerLevel * (float)(Layers - 1);
	FLinearColor Result = Base * Boost;
	Result.R = FMath::Min(Result.R, 1.f);
	Result.G = FMath::Min(Result.G, 1.f);
	Result.B = FMath::Min(Result.B, 1.f);
	Result.A = 1.f;
	return Result;
}

void URHPlayerPanelWidget::RefreshHealth()
{
	if (!BoundASC || !HPBar)
	{
		return;
	}

	const float Health = BoundASC->GetAttributeValue(UKnsCommonAttributeSet::GetHealthAttribute());
	const float MaxHealth = BoundASC->GetAttributeValue(UKnsCommonAttributeSet::GetMaxHealthAttribute());
	HPBar->SetPercent(MaxHealth > 0.f ? FMath::Clamp(Health / MaxHealth, 0.f, 1.f) : 0.f);
}
