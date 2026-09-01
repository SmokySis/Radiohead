#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "KnsPlayerAttributeSet.generated.h"

#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

UCLASS()
class DEMO_API UKnsPlayerAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UKnsPlayerAttributeSet();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, meta = (ToolTip = "当前翔虫数量"))
	FGameplayAttributeData Onom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, meta = (ToolTip = "翔虫上限"))
	FGameplayAttributeData MaxOnom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, meta = (ToolTip = "Current Focus charge"))
	FGameplayAttributeData Focus;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, meta = (ToolTip = "Maximum Focus charge"))
	FGameplayAttributeData MaxFocus;

	ATTRIBUTE_ACCESSORS(UKnsPlayerAttributeSet, Onom);
	ATTRIBUTE_ACCESSORS(UKnsPlayerAttributeSet, MaxOnom);
	ATTRIBUTE_ACCESSORS(UKnsPlayerAttributeSet, Focus);
	ATTRIBUTE_ACCESSORS(UKnsPlayerAttributeSet, MaxFocus);

protected:
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
