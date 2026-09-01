#include "RHEnemyFloatPanelWidget.h"

#include "AbilitySystemInterface.h"
#include "Demo/GAS/KnsAbilitySystemComponent.h"
#include "Demo/GAS/KnsCommonAttributeSet.h"
#include "GameplayEffectTypes.h"

void URHEnemyFloatPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();
	RefreshBars();
}

void URHEnemyFloatPanelWidget::BindTarget(AActor* InTarget)
{
	if (BoundASC)
	{
		BoundASC->GetGameplayAttributeValueChangeDelegate(UKnsCommonAttributeSet::GetHealthAttribute())
			.RemoveAll(this);
		BoundASC->GetGameplayAttributeValueChangeDelegate(UKnsCommonAttributeSet::GetResonanceAttribute())
			.RemoveAll(this);
		BoundASC = nullptr;
	}

	if (!InTarget)
	{
		return;
	}

	if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(InTarget))
	{
		BoundASC = Cast<UKnsAbilitySystemComponent>(ASI->GetAbilitySystemComponent());
	}

	if (!BoundASC)
	{
		return;
	}

	BoundASC->GetGameplayAttributeValueChangeDelegate(UKnsCommonAttributeSet::GetHealthAttribute())
		.AddUObject(this, &URHEnemyFloatPanelWidget::HandleHealthChanged);
	BoundASC->GetGameplayAttributeValueChangeDelegate(UKnsCommonAttributeSet::GetResonanceAttribute())
		.AddUObject(this, &URHEnemyFloatPanelWidget::HandleResonanceChanged);

	RefreshBars();
}

void URHEnemyFloatPanelWidget::HandleHealthChanged(const FOnAttributeChangeData& Data)
{
	RefreshBars();
}

void URHEnemyFloatPanelWidget::HandleResonanceChanged(const FOnAttributeChangeData& Data)
{
	RefreshBars();
}

void URHEnemyFloatPanelWidget::RefreshBars()
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
