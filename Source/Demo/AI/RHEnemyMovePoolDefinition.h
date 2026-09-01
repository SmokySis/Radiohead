#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Demo/AI/RHEnemyTypes.h"
#include "RHEnemyMovePoolDefinition.generated.h"

/** 敌人近战招式池：起手式 + 中间招式段 + 收尾式 + 特殊招式（特殊只在收尾后派生）+ 派生概率参数。 */
UCLASS(BlueprintType)
class DEMO_API URHEnemyMovePoolDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** 起手式池：进入攻击时按距离区间 + 权重选一个（禁止特殊招式起手）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move|Opener")
	TArray<FRHEnemyOpenerEntry> Openers;

	/** 中间招式段池：起手式播完后按权重选一条，整段播完再选收尾式。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move|Middle")
	TArray<FRHEnemyComboChain> MiddleMoves;

	/** 收尾式池：中间段播完后按当前距离 + 权重选一个。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move|Finisher")
	TArray<FRHEnemyFinisherEntry> Finishers;

	/** 特殊招式池：允许作为收尾式（收尾阶段掷骰命中即播，未命中走传统收尾式且不累加保底）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move|Special")
	TArray<FRHEnemySpecialEntry> SpecialMoves;

	/** 收尾式阶段选择特殊招式作为收尾式的概率（固定值，未命中不累加保底）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move|Chance", meta = (ClampMin = "0", ClampMax = "1"))
	float ComboEndSpecialChance = 0.15f;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
};
