#include "KnsCommonAttributeSet.h"

#include "Net/UnrealNetwork.h"

void UKnsCommonAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHealth());
	}
	else if (Attribute == GetStaminaAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxStamina());
	}
	else if (Attribute == GetPoiseAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxPoise());
	}
	else if (Attribute == GetResonanceAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxResonance());
	}
}

void UKnsCommonAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UKnsCommonAttributeSet, Health);
	DOREPLIFETIME(UKnsCommonAttributeSet, MaxHealth);
	DOREPLIFETIME(UKnsCommonAttributeSet, Stamina);
	DOREPLIFETIME(UKnsCommonAttributeSet, MaxStamina);
	DOREPLIFETIME(UKnsCommonAttributeSet, AttackPower);
	DOREPLIFETIME(UKnsCommonAttributeSet, CritRate);
	DOREPLIFETIME(UKnsCommonAttributeSet, Resistance);
	DOREPLIFETIME(UKnsCommonAttributeSet, Defense);
	DOREPLIFETIME(UKnsCommonAttributeSet, DamageReduction);
	DOREPLIFETIME(UKnsCommonAttributeSet, Poise);
	DOREPLIFETIME(UKnsCommonAttributeSet, MaxPoise);
	DOREPLIFETIME(UKnsCommonAttributeSet, Resonance);
	DOREPLIFETIME(UKnsCommonAttributeSet, MaxResonance);
}

UKnsCommonAttributeSet::UKnsCommonAttributeSet()
{
	InitMaxHealth(100.f);
	InitHealth(100.f);
	InitMaxStamina(100.f);
	InitStamina(100.f);
	InitAttackPower(10.f);
	InitCritRate(0.1f);
	InitResistance(0.f);
	InitDefense(0.f);
	InitDamageReduction(0.f);
	InitMaxPoise(100.f);
	InitPoise(100.f);
	InitMaxResonance(100.f);
	InitResonance(0.f);
}
