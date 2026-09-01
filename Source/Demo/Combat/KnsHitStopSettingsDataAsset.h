#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraShakeBase.h"
#include "Core/CameraRigAsset.h"
#include "Engine/DataAsset.h"
#include "KnsHitStopSettingsDataAsset.generated.h"

USTRUCT(BlueprintType)
struct FHitStopLevelSettings
{
	GENERATED_BODY()

	// 卡肉等级编号
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitStop", meta = (ToolTip = "卡肉等级编号"))
	int32 Level = 0;

	// 卡肉持续时长（秒）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitStop", meta = (ClampMin = "0.0", ToolTip = "卡肉持续时长（秒）"))
	float Duration = 0.1f;

	// 卡肉期间蒙太奇播放速率
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitStop", meta = (ClampMin = "0.01", ToolTip = "卡肉期间蒙太奇播放速率"))
	float MontagePlayRate = 0.3f;

	// 卡肉时摄像机震动强度，0 表示不震
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitStop", meta = (ClampMin = "0.0", ClampMax = "1.0", ToolTip = "卡肉时摄像机震动强度，0 表示不震"))
	float CameraShakeStrength = 0.f;

	// 命中时播放的旧版 CameraShakeBase，留空不播放
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitStop", meta = (ToolTip = "命中时播放的旧版 CameraShakeBase，留空不播放"))
	TSubclassOf<UCameraShakeBase> CameraShake;

	/** Camera rig activated on the player's GameplayCamera component when this level lands. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitStop|Camera")
	TObjectPtr<UCameraRigAsset> CameraRig;

	/** How long the camera rig stays active. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitStop|Camera", meta = (ClampMin = "0"))
	float CameraRigDuration = 0.1f;

	// 手柄震动强度（0-1），0 表示不振
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitStop", meta = (ClampMin = "0.0", ClampMax = "1.0", ToolTip = "手柄震动强度，0 表示不振"))
	float RumbleIntensity = 0.f;

	// 手柄震动时长（秒），0 表示与卡肉时长一致
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitStop", meta = (ClampMin = "0.0", ToolTip = "手柄震动时长（秒），0 表示与卡肉时长一致"))
	float RumbleDuration = 0.f;

	// 马达开关：左大（低频重震）/ 左小 / 右大 / 右小
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitStop")
	bool bAffectsLeftLarge = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitStop")
	bool bAffectsLeftSmall = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitStop")
	bool bAffectsRightLarge = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitStop")
	bool bAffectsRightSmall = false;
};

UCLASS(BlueprintType)
class DEMO_API UKnsHitStopSettingsDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// 所有卡肉等级的配置
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitStop", meta = (ToolTip = "所有卡肉等级的配置"))
	TArray<FHitStopLevelSettings> Levels;


	UFUNCTION(BlueprintPure, Category = "HitStop", meta = (ToolTip = "按等级查找卡肉配置，找不到返回 false"))
	bool GetSettings(int32 Level, FHitStopLevelSettings& OutSettings) const;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
};
