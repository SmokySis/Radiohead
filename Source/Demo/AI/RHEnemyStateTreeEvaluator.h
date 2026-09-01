#pragma once

#include "CoreMinimal.h"
#include "StateTreeEvaluatorBase.h"
#include "RHEnemyTypes.h"
#include "RHEnemyStateTreeEvaluator.generated.h"

class AAIController;
class ARHEnemyBase;
struct FStateTreeExecutionContext;

/** Enemy Context Evaluator 的实例数据：每帧把玩家快照和敌人资源状态写进来，编辑器里可绑定/调试。 */
USTRUCT()
struct FRHEnemyContextEvaluatorInstanceData
{
	GENERATED_BODY()

	/** 上下文：由 StateTree 编辑器自动绑定到当前 AI 控制器。 */
	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AAIController> AIController;

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<ARHEnemyBase> EnemyPawn;

	UPROPERTY(EditAnywhere, Category = "Output")
	TObjectPtr<AActor> PlayerTarget;

	UPROPERTY(EditAnywhere, Category = "Output")
	FRHPlayerCombatSnapshot Snapshot;

	UPROPERTY(EditAnywhere, Category = "Output")
	float CounterBarPercent = 0.f;

	UPROPERTY(EditAnywhere, Category = "Output")
	bool bCounterBarEmpty = false;

	UPROPERTY(EditAnywhere, Category = "Output")
	bool bResonanceBroken = false;

	UPROPERTY(EditAnywhere, Category = "Output")
	bool bIsDowned = false;

	/** 当前阶段号（1-based：1 = 基础）。 */
	UPROPERTY(EditAnywhere, Category = "Output")
	int32 CurrentPhaseIndex = 1;

	UPROPERTY(EditAnywhere, Category = "Output")
	float ResonancePercent = 0.f;
};

/**
 * Enemy Context Evaluator：
 * 状态树的每帧入口——刷新敌人 AI 组件的玩家快照，并把快照/反击条/共振/阶段写入实例数据，
 * 供条件、任务和蓝图绑定（任务/条件也可以继续直接读 AI 组件，开箱即用）。
 */
USTRUCT(meta = (DisplayName = "Enemy Context", Category = "Enemy"))
struct FRHEnemyContextEvaluator : public FStateTreeEvaluatorCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FRHEnemyContextEvaluatorInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual void TreeStart(FStateTreeExecutionContext& Context) const override;
	virtual void Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
};
