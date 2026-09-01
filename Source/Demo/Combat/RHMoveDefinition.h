#pragma once

#include "CoreMinimal.h"
#include "Demo/Onom/RHOnomTypes.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "RHMoveDefinition.generated.h"

class UAnimMontage;

UCLASS(BlueprintType)
class DEMO_API URHMoveDefinition : public UPrimaryDataAsset
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move")
	FGameplayTag AttackTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move")
	FGameplayTagContainer GrantedTags;

	/** AI intent tags (Melee/Ranged/Heal), mounted on ASC while playing so enemy AI can read. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Move")
	FGameplayTagContainer AIIntentTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move|Damage", meta = (ClampMin = "0"))
	float ActionValue = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move|Damage", meta = (ClampMin = "0"))
	float ResonanceDamage = 0.f;

	// 该招式对敌人反击条的扣减值（负数 = 给敌人回反击条）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move|Damage", meta = (ToolTip = "该招式对敌人反击条的扣减值（负数 = 给敌人回反击条）"))
	float CounterBarDamage = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move|HitStop", meta = (ClampMin = "0"))
	int32 HitStopLevel = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move|Poise", meta = (ClampMin = "0"))
	int32 PoiseLevel = 0;

	/** 执行该招式时被命中受到的减伤系数（0~1，独立于打断判定）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move|Poise", meta = (ClampMin = "0", ClampMax = "1"))
	float Resistance = 0.f;

	/** Attack poise damage level. Falls back to PoiseLevel when left at 0. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move|Poise", meta = (ClampMin = "0"))
	int32 HitPoise = 0;

	int32 GetHitPoise() const { return HitPoise > 0 ? HitPoise : PoiseLevel; }

	/** 覆盖本招式命中时攻击方获得的 Onom 规则（普攻/派生招式级自选音形）；false = 跟随武器 AttackHitRule。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move|Onom")
	bool bOverrideHitOnom = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move|Onom", meta = (EditCondition = "bOverrideHitOnom", EditConditionHides))
	FRHOnomSourceRule HitOnomRule;
};
