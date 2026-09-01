#pragma once

#include "CoreMinimal.h"
#include "Demo/Combat/KnsHitReactionTypes.h"
#include "Engine/DataAsset.h"
#include "KnsHitReactionThresholdDataAsset.generated.h"

/**
 * 受击播放阈值 DA（独立于受击蒙太奇，战斗组件直接引用）：
 * 韧性差值达到哪个阈值就归为哪个强度（轻 1 / 中 2 / 重 3 / 倒地 4，策划可调）。
 */
UCLASS(BlueprintType)
class DEMO_API UKnsHitReactionThresholdDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** 韧性差值达到该值时为对应强度。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitReaction|Poise", meta = (ClampMin = "1"))
	int32 LightPoiseDiff = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitReaction|Poise", meta = (ClampMin = "1"))
	int32 MediumPoiseDiff = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitReaction|Poise", meta = (ClampMin = "1"))
	int32 HeavyPoiseDiff = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitReaction|Poise", meta = (ClampMin = "1"))
	int32 KnockdownPoiseDiff = 4;

	UFUNCTION(BlueprintPure, Category = "HitReaction")
	EKnsHitReactionStrength GetStrengthForPoiseDiff(int32 Diff) const;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
};
