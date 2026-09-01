#pragma once

#include "CoreMinimal.h"
#include "Demo/Combat/KnsHitReactionTypes.h"
#include "Engine/DataAsset.h"
#include "KnsHitReactionSettingsDataAsset.generated.h"

class UAnimMontage;

/** 按受击类型配置一个蒙太奇；Light/Medium/Heavy 用 F/L/R/B 四个 section 区分方向，Knockdown 无 section 整段播放。 */
USTRUCT(BlueprintType)
struct FHitReactionTypeRow
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitReaction")
	EKnsHitReactionStrength Strength = EKnsHitReactionStrength::Light;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitReaction")
	TSoftObjectPtr<UAnimMontage> Montage;
};

/**
 * 受击动画 DA（与武器绑定，由武器 DA 引用，受击时按当前武器查询）：
 * - 常态受击：按类型配一个蒙太奇，蒙太奇内用 F/L/R/B section 区分方向，内部处理。
 * - 防御态：防御受击（轻） / 破防受击（中/重），同样 F/L/R/B section。
 * - 超大受击（Knockdown）：无 section 直接播放，受击瞬间受击者面向攻击方向，蒙太奇向后飞出。
 * 韧性播放阈值已拆到单独的 UKnsHitReactionThresholdDataAsset（由战斗组件直接引用）。
 */
UCLASS(BlueprintType)
class DEMO_API UKnsHitReactionSettingsDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitReaction")
	TArray<FHitReactionTypeRow> Reactions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitReaction|Defensive")
	TSoftObjectPtr<UAnimMontage> DefensiveHitMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitReaction|Defensive")
	TSoftObjectPtr<UAnimMontage> DefensiveBreakMontage;

	UFUNCTION(BlueprintPure, Category = "HitReaction")
	UAnimMontage* GetReactionMontage(EKnsHitReactionStrength Strength, EKnsHitDirection Direction) const;

	UFUNCTION(BlueprintPure, Category = "HitReaction")
	UAnimMontage* GetDefensiveHitMontage() const;

	UFUNCTION(BlueprintPure, Category = "HitReaction")
	UAnimMontage* GetDefensiveBreakMontage() const;

	/** 方向 -> section：F / L / R / B（L 对应敌人左侧、R 对应右侧，不要反）。 */
	UFUNCTION(BlueprintPure, Category = "HitReaction")
	static FName GetSectionNameForDirection(EKnsHitDirection Direction);

	FString GetDebugSummary() const;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
};
