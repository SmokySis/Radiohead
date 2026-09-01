#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "Blueprint/UserWidget.h"
#include "Components/ProgressBar.h"
#include "GameplayEffectTypes.h"
#include "Demo/Onom/RHOnomComponent.h"
#include "Demo/Onom/RHOnomActionDefinition.h"
#include "Demo/Onom/RHWeaponDefinition.h"
#include "Demo/UI/RHChargeGaugeWidget.h"
#include "Demo/UI/RHOnomSkillPreviewWidget.h"
#include "Demo/UI/RHOnomSlotWidget.h"
#include "RHPlayerPanelWidget.generated.h"

class UKnsAbilitySystemComponent;

UCLASS()
class DEMO_API URHPlayerPanelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Onom|UI")
	void BindOnomComponent(URHOnomComponent* InOnom);

	UFUNCTION(BlueprintCallable, Category = "Onom|UI")
	void BindHealthSource(AActor* InActor);

	/** 绑定当前武器：用于刷新战技预览（名称/消耗/可释放性）。 */
	UFUNCTION(BlueprintCallable, Category = "Onom|UI")
	void BindWeaponDefinition(URHWeaponDefinition* Weapon);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Onom|UI")
	FLinearColor PositiveResonanceStartColor = FLinearColor(1.f, 0.72f, 0.3f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Onom|UI")
	FLinearColor PositiveResonanceEndColor = FLinearColor(1.f, 0.1f, 0.05f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Onom|UI")
	FLinearColor NegativeResonanceStartColor = FLinearColor(0.4f, 0.65f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Onom|UI")
	FLinearColor NegativeResonanceEndColor = FLinearColor(0.1f, 0.25f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Onom|UI")
	FLinearColor NeutralResonanceColor = FLinearColor(1.f, 0.85f, 0.25f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Onom|UI", meta = (ClampMin = "0"))
	float LayerBrightenPerLevel = 0.12f;

protected:
	virtual void NativeConstruct() override;

	UFUNCTION()
	void HandleOnomStateChanged(const FRHOnomState& State);

	void RefreshSkillPreviews();
	void HandleHealthChanged(const FOnAttributeChangeData& Data);

	void RefreshHealth();
	FLinearColor GetResonanceColor(int32 Layers, ERHOnomPolarity Type) const;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> HPBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<URHOnomSlotWidget> OnomSlot0;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<URHOnomSlotWidget> OnomSlot1;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<URHOnomSlotWidget> OnomSlot2;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> ResonanceBar;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<URHOnomSkillPreviewWidget> SkillPreview0;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<URHOnomSkillPreviewWidget> SkillPreview1;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<URHOnomSkillPreviewWidget> SkillPreview2;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<URHChargeGaugeWidget> ChargeGauge;

	UPROPERTY(Transient)
	TObjectPtr<URHOnomComponent> BoundOnomComponent;

	UPROPERTY(Transient)
	TObjectPtr<URHWeaponDefinition> BoundWeaponDefinition;

	UPROPERTY(Transient)
	TObjectPtr<UKnsAbilitySystemComponent> BoundASC;
};
