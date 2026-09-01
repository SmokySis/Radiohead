#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RHOnomSettings.generated.h"

UCLASS(BlueprintType)
class DEMO_API URHOnomSettings : public UDataAsset
{
	GENERATED_BODY()

public:
	URHOnomSettings();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Onom", meta = (ClampMin = "1"))
	int32 SlotCount = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Onom", meta = (ClampMin = "0"))
	int32 MaxResonanceLayers = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Onom|Resonance")
	TArray<float> ResonanceDecaySeconds;

	/** 共鸣等级加成系数：Lv.1 / Lv.2 / Lv.3 = 1.1 / 1.5 / 1.8。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Onom|Resonance")
	TArray<float> ResonanceLevelFactors;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Onom|Charge", meta = (ClampMin = "0"))
	float ChargePercentPerOnom = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Onom|Charge", meta = (ClampMin = "0"))
	float ChargeMaxPercent = 100.f;

	UFUNCTION(BlueprintPure, Category = "Onom|Resonance")
	float GetResonanceDecayForLayer(int32 Layer) const;

	UFUNCTION(BlueprintPure, Category = "Onom|Resonance")
	float GetResonanceLevelFactor(int32 Level) const;
};
