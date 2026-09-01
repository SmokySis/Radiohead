#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "StateTreeExecutionTypes.h"
#include "Tasks/StateTreeAITask.h"
#include "RHEnemyTypes.h"
#include "RHEnemyStateTreeTasks.generated.h"

class AAIController;
class ARHEnemyBase;
class URHEnemyAIComponent;
class URHEnemyCombatComponent;
struct FStateTreeExecutionContext;

/** Idle / Guard 共用的自由走动状态：0=左走，1=右走，2=后退（后退最多 1~2s）。 */
USTRUCT()
struct FRHEnemyWanderState
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	int32 MoveMode = 0;

	UPROPERTY(Transient)
	float MoveTimer = 0.f;
};

// ---------------------------------------------------------------------------
// Approach
// ---------------------------------------------------------------------------
USTRUCT()
struct FRHEnemyApproachTaskInstanceData
{
	GENERATED_BODY()

	/** 上下文：由 StateTree 编辑器自动绑定到当前 AI 控制器。 */
	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AAIController> AIController;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	float AcceptanceRadius = 150.f;

	/** 距离超出接受半径这么多仍算到达，避免导航停在接受半径外一点点就卡死。 */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0"))
	float AcceptanceTolerance = 80.f;

	/** 内部计时：超过该秒数仍未追到玩家则 Failed（0 = 不限制）。 */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0"))
	float TimeLimitSeconds = 5.f;

	UPROPERTY(Transient)
	float LastMoveRequestTime = -1.f;

	/** Approach 内部累计计时（玩家喝药时暂停累计）。 */
	UPROPERTY(Transient)
	float TimeElapsed = 0.f;

	/** MoveToActor 寻路失败（如没有导航网格）时转直线移动兜底。 */
	UPROPERTY(Transient)
	bool bUsingDirectMovement = false;

	/** 远距离先向前突进一次（每轮 Approach 只突进一次）。 */
	UPROPERTY(Transient)
	bool bForwardDodgeDone = false;

	UPROPERTY(Transient)
	float Elapsed = 0.f;

	UPROPERTY(Transient)
	double LastTickLogTime = -1.0;
};

USTRUCT(meta = (DisplayName = "Enemy Approach", Category = "Enemy"))
struct FRHEnemyApproachTask : public FStateTreeAITaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FRHEnemyApproachTaskInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};

// ---------------------------------------------------------------------------
// Attack（近战招式池：起手式 + 中间招式段 + 收尾式组合链）
// ---------------------------------------------------------------------------
USTRUCT()
struct FRHEnemyAttackTaskInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AAIController> AIController;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	bool bResetCounterBarOnComplete = true;

	/** 当前近战组合链与进度（PickCombo 组装 起手+中间+收尾，按顺序逐段播放）。 */
	UPROPERTY(Transient)
	TArray<URHEnemyMoveDefinition*> CurrentChain;

	UPROPERTY(Transient)
	int32 CurrentStep = 0;

	UPROPERTY(Transient)
	double LastTickLogTime = -1.0;
};

USTRUCT(meta = (DisplayName = "Enemy Attack", Category = "Enemy"))
struct FRHEnemyAttackTask : public FStateTreeAITaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FRHEnemyAttackTaskInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};

// ---------------------------------------------------------------------------
// Remote Attack（远程招式：冷却制，冷却中直接 Failed 交给 approach）
// ---------------------------------------------------------------------------
USTRUCT()
struct FRHEnemyRemoteAttackTaskInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AAIController> AIController;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	bool bResetCounterBarOnComplete = true;

	UPROPERTY(Transient)
	double LastTickLogTime = -1.0;
};

USTRUCT(meta = (DisplayName = "Enemy Remote Attack", Category = "Enemy"))
struct FRHEnemyRemoteAttackTask : public FStateTreeAITaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FRHEnemyRemoteAttackTaskInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};

// ---------------------------------------------------------------------------
// Aggressive（远程/近战招式池统一挑选与释放）
// ---------------------------------------------------------------------------
USTRUCT()
struct FRHEnemyAggressiveTaskInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AAIController> AIController;

	UPROPERTY(Transient)
	TArray<URHEnemyMoveDefinition*> CurrentChain;

	UPROPERTY(Transient)
	int32 CurrentStep = 0;

	UPROPERTY(Transient)
	bool bUsingRemote = false;

	UPROPERTY(Transient)
	bool bCurrentIsSpecial = false;

	/** 连段阶段：0=None 1=Opener 2=Middle 3=Finisher 4=Special 5=Remote（单段）。 */
	UPROPERTY(Transient)
	uint8 ComboPhase = 0;

	/** 收尾式是否已播放（避免收尾阶段重复选招）。 */
	UPROPERTY(Transient)
	bool bFinisherPlayed = false;

	UPROPERTY(Transient)
	double LastTickLogTime = -1.0;
};

USTRUCT(meta = (DisplayName = "Enemy Aggressive", Category = "Enemy"))
struct FRHEnemyAggressiveTask : public FStateTreeAITaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FRHEnemyAggressiveTaskInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};

// ---------------------------------------------------------------------------
// Special
// ---------------------------------------------------------------------------
USTRUCT()
struct FRHEnemySpecialTaskInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AAIController> AIController;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	bool bResetCounterBarOnComplete = true;
};

USTRUCT(meta = (DisplayName = "Enemy Special", Category = "Enemy"))
struct FRHEnemySpecialTask : public FStateTreeAITaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FRHEnemySpecialTaskInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};

// ---------------------------------------------------------------------------
// Guard
// ---------------------------------------------------------------------------
USTRUCT()
struct FRHEnemyGuardTaskInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AAIController> AIController;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	float MaxGuardDuration = 5.f;

	/** 玩家停止攻击 tag 后继续守卫的宽限期（秒），期间若重新攻击则不清除守卫。 */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0"))
	float PlayerStopGraceSeconds = 1.f;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	bool bEndWhenPlayerStopsAttacking = true;

	UPROPERTY(Transient)
	float Elapsed = 0.f;

	UPROPERTY(Transient)
	bool bWaitingForPlayerStop = false;

	UPROPERTY(Transient)
	float StopGraceElapsed = 0.f;

	/** Guard 期间的自由走动状态（逻辑同 Idle）。 */
	UPROPERTY(Transient)
	FRHEnemyWanderState Wander;
};

USTRUCT(meta = (DisplayName = "Enemy Guard", Category = "Enemy"))
struct FRHEnemyGuardTask : public FStateTreeAITaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FRHEnemyGuardTaskInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};

// ---------------------------------------------------------------------------
// Deflect（弹开窗口）
// ---------------------------------------------------------------------------
USTRUCT()
struct FRHEnemyDeflectTaskInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AAIController> AIController;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	bool bResetCounterBarOnSuccess = true;
};

USTRUCT(meta = (DisplayName = "Enemy Deflect", Category = "Enemy"))
struct FRHEnemyDeflectTask : public FStateTreeAITaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FRHEnemyDeflectTaskInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};

// ---------------------------------------------------------------------------
// Dodge
// ---------------------------------------------------------------------------
USTRUCT()
struct FRHEnemyDodgeTaskInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AAIController> AIController;

	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0"))
	float FrontWeight = 0.f;

	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0"))
	float BackWeight = 1.f;

	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0"))
	float LeftWeight = 0.f;

	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0"))
	float RightWeight = 0.f;

	/** 未配闪避蒙太奇时的兜底位移（有根运动蒙太奇时无效）。 */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	float DodgeDistance = 250.f;

	UPROPERTY(Transient)
	float Elapsed = 0.f;

	UPROPERTY(Transient)
	bool bUsingMontage = false;
};

USTRUCT(meta = (DisplayName = "Enemy Dodge", Category = "Enemy"))
struct FRHEnemyDodgeTask : public FStateTreeAITaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FRHEnemyDodgeTaskInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};

// ---------------------------------------------------------------------------
// Down（破防硬直 -> 倒地）
// ---------------------------------------------------------------------------
USTRUCT()
struct FRHEnemyDownTaskInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AAIController> AIController;
};

USTRUCT(meta = (DisplayName = "Enemy Break / Down", Category = "Enemy"))
struct FRHEnemyDownTask : public FStateTreeAITaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FRHEnemyDownTaskInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};

// ---------------------------------------------------------------------------
// Execution
// ---------------------------------------------------------------------------
USTRUCT()
struct FRHEnemyExecutionTaskInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AAIController> AIController;

	UPROPERTY(Transient)
	double LastTickLogTime = -1.0;
};

USTRUCT(meta = (DisplayName = "Enemy Execution", Category = "Enemy"))
struct FRHEnemyExecutionTask : public FStateTreeAITaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FRHEnemyExecutionTaskInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};

// ---------------------------------------------------------------------------
// Phase Transition
// ---------------------------------------------------------------------------
USTRUCT()
struct FRHEnemyPhaseTransitionTaskInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AAIController> AIController;

	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0"))
	int32 TargetPhaseIndex = 1;
};

USTRUCT(meta = (DisplayName = "Enemy Phase Transition", Category = "Enemy"))
struct FRHEnemyPhaseTransitionTask : public FStateTreeAITaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FRHEnemyPhaseTransitionTaskInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};

// ---------------------------------------------------------------------------
// Wait Recover（受击恢复：等受击动画/硬直结束）
// ---------------------------------------------------------------------------
USTRUCT()
struct FRHEnemyWaitRecoverTaskInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AAIController> AIController;

	UPROPERTY(Transient)
	double LastTickLogTime = -1.0;
};

USTRUCT(meta = (DisplayName = "Enemy Wait Recover", Category = "Enemy"))
struct FRHEnemyWaitRecoverTask : public FStateTreeAITaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FRHEnemyWaitRecoverTaskInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};

// ---------------------------------------------------------------------------
// Idle（待机/试探，永不站桩）
// ---------------------------------------------------------------------------
USTRUCT()
struct FRHEnemyIdleTaskInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AAIController> AIController;

	/** 0 = 无限等待（靠条件/事件切走）。 */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0"))
	float Duration = 0.f;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	bool bFacePlayer = true;

	UPROPERTY(Transient)
	float Elapsed = 0.f;

	/** 自由走动状态（永不站立：左/右/后退循环）。 */
	UPROPERTY(Transient)
	FRHEnemyWanderState Wander;

	UPROPERTY(Transient)
	double LastTickLogTime = -1.0;
};

USTRUCT(meta = (DisplayName = "Enemy Idle", Category = "Enemy"))
struct FRHEnemyIdleTask : public FStateTreeAITaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FRHEnemyIdleTaskInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};

// ---------------------------------------------------------------------------
// Death
// ---------------------------------------------------------------------------
USTRUCT()
struct FRHEnemyDeathTaskInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AAIController> AIController;

	UPROPERTY(Transient)
	float Elapsed = 0.f;

	UPROPERTY(Transient)
	bool bDeathMontageStarted = false;
};

USTRUCT(meta = (DisplayName = "Enemy Death", Category = "Enemy"))
struct FRHEnemyDeathTask : public FStateTreeAITaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FRHEnemyDeathTaskInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
};

// ---------------------------------------------------------------------------
// Debug Log（验证树是否在跑）
// ---------------------------------------------------------------------------
USTRUCT()
struct FRHEnemyDebugLogTaskInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Parameter")
	FString Message = TEXT("Enemy AI tick");
};

USTRUCT(meta = (DisplayName = "Enemy Debug Log", Category = "Enemy"))
struct FRHEnemyDebugLogTask : public FStateTreeAITaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FRHEnemyDebugLogTaskInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
};
