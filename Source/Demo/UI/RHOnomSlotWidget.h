#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Image.h"
#include "Demo/Onom/RHOnomTypes.h"
#include "RHOnomSlotWidget.generated.h"

UCLASS()
class DEMO_API URHOnomSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Onom|UI")
	void SetSlotState(ERHOnomValue State);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Onom|UI")
	FLinearColor EmptyColor = FLinearColor(0.15f, 0.15f, 0.15f, 0.35f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Onom|UI")
	FLinearColor PositiveColor = FLinearColor(1.f, 0.55f, 0.15f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Onom|UI")
	FLinearColor NegativeColor = FLinearColor(0.3f, 0.6f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Onom|UI")
	FLinearColor BrokenColor = FLinearColor(0.45f, 0.45f, 0.45f);

	/** 平调音形颜色（Neutral，自定义改这个即可）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Onom|UI")
	FLinearColor NeutralColor = FLinearColor(0.85f, 0.85f, 1.f);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> SlotIcon;
};
