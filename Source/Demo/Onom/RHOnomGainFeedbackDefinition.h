#pragma once

#include "CoreMinimal.h"
#include "Demo/Onom/RHOnomTypes.h"
#include "Engine/DataAsset.h"
#include "Sound/SoundBase.h"
#include "RHOnomGainFeedbackDefinition.generated.h"

/** 单条 Onom 获得反馈：直接按 ERHOnomPolarity（大调/小调/平调/破碎）配音效。 */
USTRUCT(BlueprintType)
struct FRHOnomGainFeedbackEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Feedback")
	ERHOnomPolarity Polarity = ERHOnomPolarity::Major;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Feedback")
	TObjectPtr<USoundBase> Sound;
};

/** 玩家获得 Onom 时的音效配置（普攻获得/弹反奖励/灰色积累等，按极性选档）。 */
UCLASS(BlueprintType)
class DEMO_API URHOnomGainFeedbackDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Feedback")
	TArray<FRHOnomGainFeedbackEntry> Entries;

	UFUNCTION(BlueprintPure, Category = "Feedback")
	bool GetEntry(ERHOnomPolarity Polarity, FRHOnomGainFeedbackEntry& OutEntry) const;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
};
