#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Conditions/StateTreeAIConditionBase.h"
#include "RHEnemyStateTreeConditions.generated.h"

class AAIController;

UENUM()
enum class ERHEnemyResourceTest : uint8
{
	/** 反击条归零（行为由玩家是否攻击决定：玩家攻击中 -> 弹开，挂机 -> 主动起手）。 */
	CounterBarEmpty,
	/** 共振值满，破防中。 */
	ResonanceBroken,
	/** 当前阶段 >= Phase。 */
	PhaseAtLeast,
	/** 敌人血量百分比 <= HealthPercent。 */
	HealthPercentBelow,
	/** 普攻招式池非空。 */
	MoveSetAvailable,
	/** 特殊招式可用（冷却结束）。 */
	SpecialAvailable,
	/** 闪避冷却完毕（可以闪避）。 */
	DodgeCooldownReady,
	/** 敌人死亡（血量 <= 0）。 */
	IsDead,
	/** 敌人正在执行动作（战斗组件忙），供根转移防重入。 */
	IsAttacking,
	PlayerHealthPercentBelow,
	PlayerWeaponIdEquals,
	/** 当前阶段 <= Phase（限制转阶段只对低阶段生效，避免血量条件反复触发）。 */
	PhaseAtMost
};

// ---------------------------------------------------------------------------
// Enemy Distance
// ---------------------------------------------------------------------------
USTRUCT()
struct FRHEnemyDistanceConditionInstanceData
{
	GENERATED_BODY()

	/** 上下文：由 StateTree 编辑器自动绑定到当前 AI 控制器。 */
	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AAIController> AIController;

	/** -1 = 不限制。 */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	float MinDistance = -1.f;

	/** -1 = 不限制。 */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	float MaxDistance = -1.f;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	bool bUse2D = true;
};

USTRUCT(meta = (DisplayName = "Enemy Distance", Category = "Enemy"))
struct FRHEnemyDistanceCondition : public FStateTreeAIConditionBase
{
	GENERATED_BODY()

	using FInstanceDataType = FRHEnemyDistanceConditionInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};

// ---------------------------------------------------------------------------
// Enemy Player State
// ---------------------------------------------------------------------------
USTRUCT()
struct FRHEnemyPlayerStateConditionInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AAIController> AIController;

	/** 只按意图判断：要求玩家快照意图匹配该 tag（支持层级匹配）；空 tag = 恒真。 */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	FGameplayTag PlayerIntent;
};

USTRUCT(meta = (DisplayName = "Enemy Player Intent", Category = "Enemy"))
struct FRHEnemyPlayerStateCondition : public FStateTreeAIConditionBase
{
	GENERATED_BODY()

	using FInstanceDataType = FRHEnemyPlayerStateConditionInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};

// ---------------------------------------------------------------------------
// Enemy Resource
// ---------------------------------------------------------------------------
USTRUCT()
struct FRHEnemyResourceConditionInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AAIController> AIController;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	ERHEnemyResourceTest Test = ERHEnemyResourceTest::CounterBarEmpty;

	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0", EditCondition = "Test == ERHEnemyResourceTest::PhaseAtLeast || Test == ERHEnemyResourceTest::PhaseAtMost"))
	int32 Phase = 1;

	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0", ClampMax = "100", EditCondition = "Test == ERHEnemyResourceTest::HealthPercentBelow"))
	float HealthPercent = 50.f;
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0", ClampMax = "100", EditCondition = "Test == ERHEnemyResourceTest::PlayerHealthPercentBelow"))
	float PlayerHealthPercent = 50.f;

	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (EditCondition = "Test == ERHEnemyResourceTest::PlayerWeaponIdEquals"))
	FName PlayerWeaponId;
};

USTRUCT(meta = (DisplayName = "Enemy Resource", Category = "Enemy"))
struct FRHEnemyResourceCondition : public FStateTreeAIConditionBase
{
	GENERATED_BODY()

	using FInstanceDataType = FRHEnemyResourceConditionInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};
