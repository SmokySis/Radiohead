#include "KnsPlayerAttributeSet.h"

#include "Net/UnrealNetwork.h"

void UKnsPlayerAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetOnomAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxOnom());
	}
	else if (Attribute == GetFocusAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxFocus());
	}
}

void UKnsPlayerAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UKnsPlayerAttributeSet, Onom);
	DOREPLIFETIME(UKnsPlayerAttributeSet, MaxOnom);
	DOREPLIFETIME(UKnsPlayerAttributeSet, Focus);
	DOREPLIFETIME(UKnsPlayerAttributeSet, MaxFocus);
}

UKnsPlayerAttributeSet::UKnsPlayerAttributeSet()
{
	InitMaxOnom(100.f);
	InitOnom(0.f);
	InitMaxFocus(100.f);
	InitFocus(0.f);
}
