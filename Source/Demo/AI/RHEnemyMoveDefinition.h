#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "RHEnemyMoveDefinition.generated.h"

class UAnimMontage;

/** 敌人普攻/连段招式：只包含敌人 AI 需要的数据，蒙太奇由策划在 DA 里配置。 */
UCLASS(BlueprintType)
class DEMO_API URHEnemyMoveDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AssetRegistrySearchable, Category = "Move")
	FName MoveId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move")
	TSoftObjectPtr<UAnimMontage> Montage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move")
	FName SectionName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move", meta = (ClampMin = "0.01"))
	float PlayRate = 1.f;

	/** 播放期间是否持续面向玩家（部分招式不用转）。默认 true。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move")
	bool bFacePlayer = true;

	/** 伤害 = AttackPower x ActionValue。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move|Damage", meta = (ClampMin = "0"))
	float ActionValue = 1.f;

	/** 招式韧性：命中时按韧性差值触发 轻/中/重/倒地 受击。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move|Poise", meta = (ClampMin = "0"))
	int32 PoiseLevel = 0;

	/** Attack poise damage level. Falls back to PoiseLevel when left at 0. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move|Poise", meta = (ClampMin = "0"))
	int32 HitPoise = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move|HitStop", meta = (ClampMin = "0"))
	int32 HitStopLevel = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move")
	FGameplayTag AttackTag;

	int32 GetHitPoise() const { return HitPoise > 0 ? HitPoise : PoiseLevel; }

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
};
