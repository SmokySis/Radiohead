#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "KnsMoveDefinition.generated.h"

class UAnimMontage;

UCLASS(BlueprintType)
class DEMO_API UKnsMoveDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// 招式唯一 ID，方便策划辨认和日志排查
	UPROPERTY(EditAnywhere, BlueprintReadOnly, AssetRegistrySearchable, Category = "Move", meta = (ToolTip = "招式唯一 ID，方便策划辨认和日志排查"))
	FName MoveId;

	// 策划可读的招式名
	UPROPERTY(EditAnywhere, BlueprintReadOnly, AssetRegistrySearchable, Category = "Move", meta = (ToolTip = "策划可读的招式名"))
	FText DisplayName;

	// 招式播放的蒙太奇
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Move", meta = (ToolTip = "招式播放的蒙太奇"))
	TSoftObjectPtr<UAnimMontage> Montage;

	// 可选：从蒙太奇指定 Section 开始
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Move", meta = (ToolTip = "可选：从蒙太奇指定 Section 开始"))
	FName SectionName;

	// 蒙太奇播放速率
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Move", meta = (ClampMin = "0.01", ToolTip = "蒙太奇播放速率"))
	float PlayRate = 1.f;

	// 该招式的攻击标识 Tag
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Move", meta = (ToolTip = "该招式的攻击标识 Tag"))
	FGameplayTag AttackTag;

	// 招式播放期间挂到角色 ASC 上的 Tag
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Move", meta = (ToolTip = "招式播放期间挂到角色 ASC 上的 Tag"))
	FGameplayTagContainer GrantedTags;

	// 动作值：招式伤害倍率
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Damage", meta = (ClampMin = "0.0", ToolTip = "动作值：招式伤害倍率"))
	float ActionValue = 1.f;

	// 该招式消耗的体力
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cost", meta = (ClampMin = "0.0", ToolTip = "该招式消耗的体力"))
	float StaminaCost = 0.f;

	// Onom cost for this move (weapon arts / spells).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cost", meta = (ClampMin = "0.0", ToolTip = "Onom cost for this move"))
	float OnomCost = 0.f;

	// 该招式的削韧值
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Damage", meta = (ClampMin = "0.0", ToolTip = "该招式的削韧值"))
	float PoiseDamage = 0.f;

	// 该招式对敌人反击条的扣减值（负数 = 给敌人回反击条）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Damage", meta = (ToolTip = "该招式对敌人反击条的扣减值（负数 = 给敌人回反击条）"))
	float CounterBarDamage = 0.f;

	// 卡肉等级，具体数值由 HitStopSettings 配置
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitStop", meta = (ClampMin = "0", ToolTip = "卡肉等级，具体数值由 HitStopSettings 配置"))
	int32 HitStopLevel = 0;

	// 招式韧性等级，待机为 0，99 表示霸体
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Poise", meta = (ClampMin = "0", ToolTip = "招式韧性等级，待机为 0，99 表示霸体"))
	int32 PoiseLevel = 0;

	void ValidateMove(TArray<FText>& OutErrors, const FName& NodeId, bool bRequiresComboWindow) const;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
};
