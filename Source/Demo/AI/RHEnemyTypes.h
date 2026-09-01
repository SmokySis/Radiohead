#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "RHEnemyTypes.generated.h"

class UAnimMontage;
class URHEnemyMoveDefinition;
class URHEnemyActionDefinition;

UENUM(BlueprintType)
enum class ERHEnemyDodgeDirection : uint8
{
	Front,
	Back,
	Left,
	Right
};

/** 玩家当前行为（由 State.Player.<行为> 互斥 tag 提取）。 */
UENUM(BlueprintType)
enum class ERHPlayerBehavior : uint8
{
	None,
	Attacking,
	Skill,
	Load,
	Toss,
	Dodge,
	Block,
	Parry
};

/** 敌人固有属性：只保留战斗需要的数值，防御/暴击等公式一律不做。 */
USTRUCT(BlueprintType)
struct FRHEnemyAttributeConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes", meta = (ClampMin = "1"))
	float MaxHealth = 1200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes", meta = (ClampMin = "0"))
	float AttackPower = 15.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes", meta = (ClampMin = "0"))
	float WalkSpeed = 420.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes", meta = (ClampMin = "0"))
	float TurnSpeed = 360.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes", meta = (ClampMin = "0"))
	float AttackRange = 260.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes", meta = (ClampMin = "0"))
	float PreferredRange = 200.f;

	/** 破防倒地后，玩家进入这个距离内用普攻才会触发处决。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes", meta = (ClampMin = "0"))
	float ExecutionRange = 180.f;
};

/** 反击条：只保存上限；扣减值由玩家招式的 CounterBarDamage 提供，归零后由状态树触发 Deflect。 */
USTRUCT(BlueprintType)
struct FRHEnemyCounterBarConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CounterBar", meta = (ClampMin = "1"))
	float Max = 100.f;
};

/** 共振值（破防/处决资源）：玩家攻击把它从 0 推到 Max，满了破防；破防后自然回落。 */
USTRUCT(BlueprintType)
struct FRHEnemyResonanceConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resonance", meta = (ClampMin = "1"))
	float MaxResonance = 100.f;

	/** 玩家共振伤害换算倍率。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resonance", meta = (ClampMin = "0"))
	float ResonanceGainScale = 1.f;

	/** 非破防时自然回落速度。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resonance", meta = (ClampMin = "0"))
	float DecayPerSecond = 3.f;

	/** 破防倒地后回落速度（决定处决窗口长短）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resonance", meta = (ClampMin = "0"))
	float DecayPerSecondDuringBreak = 20.f;

	/** 处决固定伤害。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resonance", meta = (ClampMin = "0"))
	float ExecutionFixedDamage = 15.f;

	/** 破防期间全程增伤系数（进入破防任务即生效，不需要 ANS）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resonance", meta = (ClampMin = "0"))
	float BreakDamageMultiplier = 1.5f;

	/** 处决倒地后的无敌保护时长（倒地不吃任何伤害）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resonance", meta = (ClampMin = "0"))
	float GetupInvincibleSeconds = 1.5f;
};

USTRUCT(BlueprintType)
struct FRHEnemyFeelConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Feel", meta = (ClampMin = "1"))
	int32 MaxComboCount = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Feel", meta = (ClampMin = "0", ClampMax = "1"))
	float SpecialMoveChance = 0.25f;

	/** Idle 横移速度（横移结束/被打断恢复 WalkSpeed）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Feel", meta = (ClampMin = "0"))
	float IdleStrafeSpeed = 200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Feel", meta = (ClampMin = "0"))
	float IdleStrafeDuration = 1.5f;

	/** Approach 时与玩家距离超过该值，先向前突进一次再奔跑接近。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Feel", meta = (ClampMin = "0"))
	float ApproachDodgeFirstThreshold = 1000.f;

	/** Minimum seconds between enemy dodges. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Feel", meta = (ClampMin = "0"))
	float DodgeCooldownSeconds = 2.f;
};

USTRUCT(BlueprintType)
struct FRHEnemyMoveEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move")
	TObjectPtr<URHEnemyMoveDefinition> Move;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move", meta = (ClampMin = "0"))
	float Weight = 1.f;
};

/** 起手式：开场先手招式，按距离区间 + 权重选择（禁止特殊招式起手）。 */
USTRUCT(BlueprintType)
struct FRHEnemyOpenerEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Opener")
	TObjectPtr<URHEnemyMoveDefinition> Move;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Opener", meta = (ClampMin = "0"))
	float Weight = 1.f;

	/** 可用距离区间：玩家距离在 [MinRange, MaxRange] 内才参与选择；MaxRange = -1 表示不限上限。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Opener", meta = (ClampMin = "0"))
	float MinRange = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Opener", meta = (ClampMin = "-1"))
	float MaxRange = -1.f;
};

/** 收尾式：中间招式段播完后按当前距离 + 权重选择。 */
USTRUCT(BlueprintType)
struct FRHEnemyFinisherEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Finisher")
	TObjectPtr<URHEnemyMoveDefinition> Move;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Finisher", meta = (ClampMin = "0"))
	float Weight = 1.f;

	/** 可用距离区间：玩家距离在 [MinRange, MaxRange] 内才参与选择；MaxRange = -1 表示不限上限。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Finisher", meta = (ClampMin = "0"))
	float MinRange = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Finisher", meta = (ClampMin = "-1"))
	float MaxRange = -1.f;
};

/** 一条中间招式段：有序的中间招式序列（按顺序打），整段参与权重选择；起手式播完后进入，整段播完再选收尾式。 */
USTRUCT(BlueprintType)
struct FRHEnemyComboChain
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo")
	TArray<FRHEnemyMoveEntry> Moves;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo", meta = (ClampMin = "0"))
	float Weight = 1.f;

	/** 可用距离区间：玩家距离在 [MinRange, MaxRange] 内才参与选择；MaxRange = -1 表示不限上限。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo", meta = (ClampMin = "0"))
	float MinRange = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo", meta = (ClampMin = "-1"))
	float MaxRange = -1.f;
};

USTRUCT(BlueprintType)
struct FRHEnemySpecialEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Special")
	TObjectPtr<URHEnemyActionDefinition> Action;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Special", meta = (ClampMin = "0"))
	float Weight = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Special", meta = (ClampMin = "0"))
	float CooldownSeconds = 8.f;

	/** 可用距离区间：玩家距离在 [MinRange, MaxRange] 内才参与选择；MaxRange = -1 表示不限上限。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Special", meta = (ClampMin = "0"))
	float MinRange = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Special", meta = (ClampMin = "-1"))
	float MaxRange = -1.f;
};

/** 转阶段配置：直接写在敌人 DA 里（每个敌人的阶段不通用）。数组第 i 项 = 第 i+1 阶段。 */
USTRUCT(BlueprintType)
struct FRHEnemyPhaseConfig
{
	GENERATED_BODY()

	/** 血量低于该百分比时进入本阶段。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Phase", meta = (ClampMin = "1", ClampMax = "100"))
	int32 EnterAtHealthPercent = 100;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Phase|Override")
	bool bOverrideAttributes = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Phase|Override", meta = (EditCondition = "bOverrideAttributes"))
	FRHEnemyAttributeConfig Attributes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Phase|Override")
	bool bOverrideCounterBar = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Phase|Override", meta = (EditCondition = "bOverrideCounterBar"))
	FRHEnemyCounterBarConfig CounterBar;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Phase|Override")
	bool bOverrideResonance = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Phase|Override", meta = (EditCondition = "bOverrideResonance"))
	FRHEnemyResonanceConfig Resonance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Phase|Override")
	bool bOverrideFeel = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Phase|Override", meta = (EditCondition = "bOverrideFeel"))
	FRHEnemyFeelConfig Feel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Phase|Override")
	bool bOverrideMeleeMoves = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Phase|Override", meta = (EditCondition = "bOverrideMeleeMoves"))
	TArray<FRHEnemyOpenerEntry> Openers;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Phase|Override", meta = (EditCondition = "bOverrideMeleeMoves"))
	TArray<FRHEnemyComboChain> MiddleMoves;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Phase|Override", meta = (EditCondition = "bOverrideMeleeMoves"))
	TArray<FRHEnemyFinisherEntry> Finishers;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Phase|Override")
	bool bOverrideSpecialMoves = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Phase|Override", meta = (EditCondition = "bOverrideSpecialMoves"))
	TArray<FRHEnemySpecialEntry> SpecialMoves;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Phase")
	TSoftObjectPtr<UAnimMontage> EntranceMontage;
};

/** 运行时解析后的完整配置（基础 + 当前阶段 Override），由敌人 DA 生成。 */
USTRUCT()
struct FRHEnemyRuntimeConfig
{
	GENERATED_BODY()

	FRHEnemyAttributeConfig Attributes;
	FRHEnemyCounterBarConfig CounterBar;
	FRHEnemyResonanceConfig Resonance;
	FRHEnemyFeelConfig Feel;
	/** 近战：起手式池（按距离+权重选，禁止特殊起手）。 */
	TArray<FRHEnemyOpenerEntry> Openers;
	/** 近战：中间招式段池（起手式后按权重选一条，整段播完再收尾）。 */
	TArray<FRHEnemyComboChain> MiddleMoves;
	/** 近战：收尾式池（中间段播完后按距离+权重选）。 */
	TArray<FRHEnemyFinisherEntry> Finishers;
	/** 特殊招式池：允许作为收尾式（收尾阶段掷骰命中即播，未命中走传统收尾式且不累加保底）。 */
	TArray<FRHEnemySpecialEntry> SpecialMoves;
	/** 远程：直接引用普通招式（固定普攻、无特殊），随机触发。 */
	TArray<TObjectPtr<URHEnemyMoveDefinition>> RemoteMoves;
	float RemoteAttackThreshold = 800.f;
	/** 远程冷却：用过一次远程后该秒数内不再触发远程（远程任务/Aggressive 远程分支直接 Failed）。 */
	float RemoteCooldownSeconds = 5.f;
	/** 收尾式阶段选择特殊招式作为收尾式的概率（固定值，未命中不累加保底）。 */
	float ComboEndSpecialChance = 0.15f;
	bool bFloatBar = false;
	TSoftObjectPtr<UAnimMontage> EntranceMontage;
	TSoftObjectPtr<UAnimMontage> DeflectMontage;
	TSoftObjectPtr<UAnimMontage> DownMontage;
	TSoftObjectPtr<UAnimMontage> GetupMontage;
	TSoftObjectPtr<UAnimMontage> DeathMontage;
	TSoftObjectPtr<UAnimMontage> DodgeMontage;
	TSoftObjectPtr<UAnimMontage> GuardMontage;
};

/** 玩家战斗快照：敌人 AI 每帧计算，状态树直接读。 */
USTRUCT(BlueprintType)
struct FRHPlayerCombatSnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Snapshot")
	TObjectPtr<AActor> Player;

	UPROPERTY(BlueprintReadOnly, Category = "Snapshot")
	float Distance = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Snapshot")
	float Distance2D = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Snapshot")
	float AngleToPlayer = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Snapshot")
	bool bPlayerAttacking = false;

	UPROPERTY(BlueprintReadOnly, Category = "Snapshot")
	ERHPlayerBehavior PlayerBehavior = ERHPlayerBehavior::None;

	UPROPERTY(BlueprintReadOnly, Category = "Snapshot")
	bool bPlayerBusy = false;

	UPROPERTY(BlueprintReadOnly, Category = "Snapshot")
	bool bPlayerBlocking = false;

	UPROPERTY(BlueprintReadOnly, Category = "Snapshot")
	bool bPlayerParrying = false;

	UPROPERTY(BlueprintReadOnly, Category = "Snapshot")
	bool bPlayerDodging = false;

	UPROPERTY(BlueprintReadOnly, Category = "Snapshot")
	bool bPlayerGuarding = false;

	UPROPERTY(BlueprintReadOnly, Category = "Snapshot")
	bool bPlayerHealing = false;

	UPROPERTY(BlueprintReadOnly, Category = "Snapshot")
	bool bPlayerOnAir = false;

	UPROPERTY(BlueprintReadOnly, Category = "Snapshot")
	FGameplayTag PlayerIntent;

	UPROPERTY(BlueprintReadOnly, Category = "Snapshot")
	float TimeSinceLastPlayerAttack = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Snapshot")
	float PlayerHealthPercent = 100.f;

	UPROPERTY(BlueprintReadOnly, Category = "Snapshot")
	FName PlayerWeaponId;
};
