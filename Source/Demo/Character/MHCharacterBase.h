#pragma once

#include "CoreMinimal.h"
#include "Demo/Character/BaseCharacter.h"
#include "Demo/Combat/KnsCombatComponent.h"
#include "Demo/Combat/KnsCombatContextComponent.h"
#include "Demo/Combo/KnsComboComponent.h"
#include "Demo/Component/KnsResourceRegenComponent.h"
#include "Demo/GAS/KnsAbilitySystemComponent.h"
#include "Demo/GAS/KnsCommonAttributeSet.h"
#include "Demo/GAS/KnsPlayerAttributeSet.h"
#include "MHCharacterBase.generated.h"

UCLASS()
class DEMO_API AMHCharacterBase : public ABaseCharacter
{
	GENERATED_BODY()

public:
	AMHCharacterBase();

	virtual void BeginPlay() override;
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MH|GAS")
	TObjectPtr<UKnsAbilitySystemComponent> MHAbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MH|GAS")
	TObjectPtr<UKnsCommonAttributeSet> MHCommonAttributeSet;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MH|GAS")
	TObjectPtr<UKnsPlayerAttributeSet> MHPlayerAttributeSet;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MH|Combo")
	TObjectPtr<UKnsComboComponent> MHComboComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MH|Resource")
	TObjectPtr<UKnsResourceRegenComponent> MHResourceRegenComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MH|Combat")
	TObjectPtr<UKnsCombatComponent> MHCombatComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MH|Combat")
	TObjectPtr<UKnsCombatContextComponent> MHCombatContextComponent;
};
