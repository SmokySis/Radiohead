#include "KnsResourceRegenComponent.h"

#include "AbilitySystemInterface.h"
#include "Demo/GAS/KnsAbilitySystemComponent.h"

UKnsResourceRegenComponent::UKnsResourceRegenComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UKnsResourceRegenComponent::BeginPlay()
{
	Super::BeginPlay();

	if (UKnsAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		LastStaminaValue = ASC->GetStamina();
		LastOnomValue = ASC->GetOnom();
		LastFocusValue = ASC->GetFocus();
	}
}

void UKnsResourceRegenComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UKnsAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC || !GetWorld())
	{
		return;
	}

	const float Now = GetWorld()->GetTimeSeconds();

	const float CurrentStamina = ASC->GetStamina();
	if (CurrentStamina < LastStaminaValue)
	{
		LastStaminaChangeTime = Now;
	}
	LastStaminaValue = CurrentStamina;

	const float CurrentOnom = ASC->GetOnom();
	if (CurrentOnom < LastOnomValue)
	{
		LastOnomChangeTime = Now;
	}
	LastOnomValue = CurrentOnom;

	const float CurrentFocus = ASC->GetFocus();
	if (CurrentFocus < LastFocusValue)
	{
		LastFocusChangeTime = Now;
	}
	LastFocusValue = CurrentFocus;

	if (bAutoRegenStamina && CurrentStamina < ASC->GetMaxStamina() && Now - LastStaminaChangeTime >= StaminaRegenDelay)
	{
		ASC->RegenerateStamina(StaminaRegenRate * DeltaTime);
	}

	if (bAutoRegenOnom && CurrentOnom < ASC->GetMaxOnom() && Now - LastOnomChangeTime >= OnomRegenDelay)
	{
		ASC->RegenerateOnom(OnomRegenRate * DeltaTime);
	}

	if (bAutoRegenFocus && CurrentFocus < ASC->GetMaxFocus() && Now - LastFocusChangeTime >= FocusRegenDelay)
	{
		ASC->RegenerateFocus(FocusRegenRate * DeltaTime);
	}
}

UKnsAbilitySystemComponent* UKnsResourceRegenComponent::GetAbilitySystemComponent() const
{
	if (AActor* Owner = GetOwner())
	{
		if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Owner))
		{
			return Cast<UKnsAbilitySystemComponent>(ASI->GetAbilitySystemComponent());
		}
	}

	return nullptr;
}
