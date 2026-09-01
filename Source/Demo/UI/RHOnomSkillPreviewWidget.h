#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Demo/Onom/RHOnomTypes.h"
#include "RHOnomSkillPreviewWidget.generated.h"

class URHOnomActionDefinition;

/** 单个战技预览：名称 + 消耗 + 当前可释放性。 */
UCLASS()
class DEMO_API URHOnomSkillPreviewWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Onom|UI")
	void SetSkill(URHOnomActionDefinition* Skill, const FRHOnomConsumptionData& Available);

	UFUNCTION(BlueprintCallable, Category = "Onom|UI")
	void ClearSkill();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Onom|UI")
	FLinearColor AvailableColor = FLinearColor(0.2f, 0.85f, 0.25f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Onom|UI")
	FLinearColor UnavailableColor = FLinearColor(0.5f, 0.15f, 0.15f);

protected:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SkillNameText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CostText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> AvailabilityText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> AvailabilityIcon;
};
