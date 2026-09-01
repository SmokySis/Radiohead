#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "KnsCommonAttributeSet.generated.h"

#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

UCLASS()
class DEMO_API UKnsCommonAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UKnsCommonAttributeSet();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, meta = (ToolTip = "当前生命值"))
	FGameplayAttributeData Health;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, meta = (ToolTip = "生命上限"))
	FGameplayAttributeData MaxHealth;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, meta = (ToolTip = "当前体力"))
	FGameplayAttributeData Stamina;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, meta = (ToolTip = "体力上限"))
	FGameplayAttributeData MaxStamina;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, meta = (ToolTip = "攻击力"))
	FGameplayAttributeData AttackPower;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, meta = (ToolTip = "暴击率，0~1"))
	FGameplayAttributeData CritRate;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, meta = (ToolTip = "抗性，0~1，按百分比减免来源伤害"))
	FGameplayAttributeData Resistance;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, meta = (ToolTip = "防御力，参与 100/(100+防御) 减伤"))
	FGameplayAttributeData Defense;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, meta = (ToolTip = "减伤系数，0~1"))
	FGameplayAttributeData DamageReduction;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, meta = (ToolTip = "当前韧性"))
	FGameplayAttributeData Poise;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, meta = (ToolTip = "韧性上限"))
	FGameplayAttributeData MaxPoise;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, meta = (ToolTip = "Current resonance value"))
	FGameplayAttributeData Resonance;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, meta = (ToolTip = "Maximum resonance value"))
	FGameplayAttributeData MaxResonance;

	ATTRIBUTE_ACCESSORS(UKnsCommonAttributeSet, Health);
	ATTRIBUTE_ACCESSORS(UKnsCommonAttributeSet, MaxHealth);
	ATTRIBUTE_ACCESSORS(UKnsCommonAttributeSet, Stamina);
	ATTRIBUTE_ACCESSORS(UKnsCommonAttributeSet, MaxStamina);
	ATTRIBUTE_ACCESSORS(UKnsCommonAttributeSet, AttackPower);
	ATTRIBUTE_ACCESSORS(UKnsCommonAttributeSet, CritRate);
	ATTRIBUTE_ACCESSORS(UKnsCommonAttributeSet, Resistance);
	ATTRIBUTE_ACCESSORS(UKnsCommonAttributeSet, Defense);
	ATTRIBUTE_ACCESSORS(UKnsCommonAttributeSet, DamageReduction);
	ATTRIBUTE_ACCESSORS(UKnsCommonAttributeSet, Poise);
	ATTRIBUTE_ACCESSORS(UKnsCommonAttributeSet, MaxPoise);
	ATTRIBUTE_ACCESSORS(UKnsCommonAttributeSet, Resonance);
	ATTRIBUTE_ACCESSORS(UKnsCommonAttributeSet, MaxResonance);

protected:
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
