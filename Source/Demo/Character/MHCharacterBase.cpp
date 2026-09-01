#include "MHCharacterBase.h"

#include "Components/CapsuleComponent.h"

AMHCharacterBase::AMHCharacterBase()
{
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Overlap);
	GetCapsuleComponent()->SetGenerateOverlapEvents(true);

	MHAbilitySystemComponent = CreateDefaultSubobject<UKnsAbilitySystemComponent>("MH_ASC");
	MHCommonAttributeSet = CreateDefaultSubobject<UKnsCommonAttributeSet>("MH_CommonAS");
	MHPlayerAttributeSet = CreateDefaultSubobject<UKnsPlayerAttributeSet>("MH_PlayerAS");
	MHComboComponent = CreateDefaultSubobject<UKnsComboComponent>("MH_Combo");
	MHResourceRegenComponent = CreateDefaultSubobject<UKnsResourceRegenComponent>("MH_ResourceRegen");
	MHCombatComponent = CreateDefaultSubobject<UKnsCombatComponent>("MH_Combat");
	MHCombatContextComponent = CreateDefaultSubobject<UKnsCombatContextComponent>("MH_CombatContext");
}

void AMHCharacterBase::BeginPlay()
{
	Super::BeginPlay();
	MHAbilitySystemComponent->InitAbilityActorInfo(this, this);
}

UAbilitySystemComponent* AMHCharacterBase::GetAbilitySystemComponent() const
{
	return MHAbilitySystemComponent;
}
