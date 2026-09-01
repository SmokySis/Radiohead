#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Demo/Onom/RHOnomTypes.h"
#include "RHEnemyActionDefinition.generated.h"

class UAnimMontage;

/** 敌人特殊招式：参考玩家 ActionDef，特点在于可选 EffectAbility（投射物/范围/增益，由 AN_CastEffect 触发）。 */
UCLASS(BlueprintType)
class DEMO_API URHEnemyActionDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AssetRegistrySearchable, Category = "Action")
	FName ActionId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action")
	TSoftObjectPtr<UAnimMontage> Montage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action")
	FName SectionName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action", meta = (ClampMin = "0.01"))
	float PlayRate = 1.f;

	/** 伤害 = AttackPower x ActionValue。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|Damage", meta = (ClampMin = "0"))
	float ActionValue = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|Poise", meta = (ClampMin = "0"))
	int32 PoiseLevel = 0;

	/** Attack poise damage level. Falls back to PoiseLevel when left at 0. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|Poise", meta = (ClampMin = "0"))
	int32 HitPoise = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|HitStop", meta = (ClampMin = "0"))
	int32 HitStopLevel = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action")
	FGameplayTag AttackTag;

	/** 施放极性：命中反馈/拖尾按此取档（和玩家战技一致）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|Effect")
	ERHOnomPolarity Polarity = ERHOnomPolarity::None;

	/** 可选效果 GA：蒙太奇上放 AN_CastEffect 触发，GA 类从这里读。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|Effect")
	TSubclassOf<UGameplayAbility> EffectAbility;

	int32 GetHitPoise() const { return HitPoise > 0 ? HitPoise : PoiseLevel; }

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
};
