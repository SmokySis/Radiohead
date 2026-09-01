#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "RHChargeGaugeWidget.generated.h"

UCLASS()
class DEMO_API URHChargeGaugeWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Onom|UI")
	void SetChargePercent(float Percent);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> ChargeBar;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ChargeText;
};
