#include "RHEnemyPanelWidget.h"

#include "AbilitySystemInterface.h"
#include "Components/ProgressBar.h"
#include "Demo/GAS/KnsAbilitySystemComponent.h"
#include "Demo/GAS/KnsCommonAttributeSet.h"

void URHEnemyPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();
	ClearTarget();
}

void URHEnemyPanelWidget::BindTarget(AActor* InTarget)
{
	if (!InTarget)
	{
		ClearTarget();
		return;
	}

	if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(InTarget))
	{
		BoundASC = Cast<UKnsAbilitySystemComponent>(ASI->GetAbilitySystemComponent());
	}

	if (!BoundASC)
	{
		ClearTarget();
		return;
	}

	BoundASC->GetGameplayAttributeValueChangeDelegate(UKnsCommonAttributeSet::GetHealthAttribute())
		.AddUObject(this, &URHEnemyPanelWidget::HandleHealthChanged);
	BoundASC->GetGameplayAttributeValueChangeDelegate(UKnsCommonAttributeSet::GetResonanceAttribute())
		.AddUObject(this, &URHEnemyPanelWidget::HandleResonanceChanged);

	SetVisibility(ESlateVisibility::Visible);
	RefreshBars();
}

void URHEnemyPanelWidget::ClearTarget()
{
	BoundASC = nullptr;
	SetVisibility(ESlateVisibility::Collapsed);
}

void URHEnemyPanelWidget::HandleHealthChanged(const FOnAttributeChangeData& Data)
{
	RefreshBars();
}

void URHEnemyPanelWidget::HandleResonanceChanged(const FOnAttributeChangeData& Data)
{
	RefreshBars();
}

void URHEnemyPanelWidget::RefreshBars()
{
	if (!BoundASC)
	{
		return;
	}

	const float Health = BoundASC->GetAttributeValue(UKnsCommonAttributeSet::GetHealthAttribute());
	const float MaxHealth = BoundASC->GetAttributeValue(UKnsCommonAttributeSet::GetMaxHealthAttribute());
	const float Resonance = BoundASC->GetAttributeValue(UKnsCommonAttributeSet::GetResonanceAttribute());
	const float MaxResonance = BoundASC->GetAttributeValue(UKnsCommonAttributeSet::GetMaxResonanceAttribute());

	if (EnemyHPBar)
	{
		EnemyHPBar->SetPercent(MaxHealth > 0.f ? FMath::Clamp(Health / MaxHealth, 0.f, 1.f) : 0.f);
	}
	if (EnemyResonanceBar)
	{
		EnemyResonanceBar->SetPercent(MaxResonance > 0.f ? FMath::Clamp(Resonance / MaxResonance, 0.f, 1.f) : 0.f);
	}
}
