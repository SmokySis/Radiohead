#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/TimerHandle.h"
#include "RHEnemyTypes.h"
#include "RHEnemyAIComponent.generated.h"

class AActor;
class UKnsAbilitySystemComponent;
class URHEnemyDefinition;
class URHEnemyMoveDefinition;
class URHEnemyActionDefinition;
class URHEnemyCombatComponent;
struct FRHHitData;

/**
 * 敌人 AI 私有状态组件（不进 UI）：
 * - 反击条（隐藏）：被玩家命中扣减、玩家挂机时自然衰减；归零后由状态树按“玩家是否攻击”决定弹开/起手。
 * - 共振值：玩家攻击从 0 推到 Max -> 破防 -> 自然回落 -> 处决/起身。
 * - 玩家战斗快照：距离/朝向/意图 tag 等，状态树条件直接读。
 */
UCLASS(ClassGroup = (AI), meta = (BlueprintSpawnableComponent))
class DEMO_API URHEnemyAIComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	URHEnemyAIComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|AI")
	TObjectPtr<URHEnemyDefinition> Definition;

	// ---- 反击条（隐藏 AI 参数） ----
	UFUNCTION(BlueprintCallable, Category = "Enemy|CounterBar")
	void ResetCounterBar();

	UFUNCTION(BlueprintPure, Category = "Enemy|CounterBar")
	float GetCounterBarPercent() const;

	UFUNCTION(BlueprintPure, Category = "Enemy|CounterBar")
	bool IsCounterBarEmpty() const;

	// ---- 共振值 / 破防 / 处决 ----
	UFUNCTION(BlueprintCallable, Category = "Enemy|Resonance")
	void AddResonance(float Amount);

	UFUNCTION(BlueprintPure, Category = "Enemy|Resonance")
	float GetResonancePercent() const;

	UFUNCTION(BlueprintPure, Category = "Enemy|Resonance")
	bool IsBroken() const;

	/** 是否处于受击硬直（Status.Staggered）。 */
	UFUNCTION(BlueprintPure, Category = "Enemy|Resonance")
	bool IsStaggered() const;

	UFUNCTION(BlueprintPure, Category = "Enemy|Resonance")
	float GetBreakDamageMultiplier() const;

	/** 破防期间受伤扣共振（预留，当前未启用：破防解除只由自然衰减或处决完成驱动）。 */
	UFUNCTION(BlueprintCallable, Category = "Enemy|Resonance")
	void ApplyBreakDamage(float Damage);

	UFUNCTION(BlueprintPure, Category = "Enemy|Resonance")
	bool IsInExecutionRange(AActor* Source) const;

	UFUNCTION(BlueprintCallable, Category = "Enemy|Resonance")
	void ExecuteEnemy(AActor* Instigator);

	/** 鐜╁鍛戒腑鏃舵墸鍑忓弽鍑绘潯锛涘綊闆舵椂鍙戝嚭 AI.Enemy.Deflect 骞惰繑鍥?true銆?*/
	UFUNCTION(BlueprintCallable, Category = "Enemy|CounterBar")
	bool DrainCounterBarOnPlayerHit(float Damage, AActor* Instigator);

	/** 澶勫喅浼ゅ钀藉湴锛堢敱澶勫喅钂欏お濂囩殑 Notify 璋冪敤锛岃蛋 ApplyFlatDamageToActor 涓嶈蛋 HitTaken锛夈€?*/
	UFUNCTION(BlueprintCallable, Category = "Enemy|Resonance")
	void ApplyExecutionDamage();

	UFUNCTION(BlueprintPure, Category = "Enemy|Resonance")
	bool HasExecutionDamageApplied() const { return bExecutionDamageApplied; }

	UFUNCTION(BlueprintCallable, Category = "Enemy|Resonance")
	void BeginExecution();

	/** 澶勫喅钂欏お濂囩粨鏉熷悗瑙ｉ櫎鐮撮槻骞惰繘鍏ヨ捣韬棤鏁屾湡銆?*/
	UFUNCTION(BlueprintCallable, Category = "Enemy|Resonance")
	void FinishExecution();

	/** 处决后是否处于倒地状态（无敌保护中，不吃任何伤害）。 */
	UFUNCTION(BlueprintPure, Category = "Enemy|Resonance")
	bool IsDowned() const { return bIsDowned; }

	// ---- 弹开窗口 ----
	UFUNCTION(BlueprintCallable, Category = "Enemy|Deflect")
	void OpenDeflectWindow();

	UFUNCTION(BlueprintCallable, Category = "Enemy|Deflect")
	void CloseDeflectWindow();

	UFUNCTION(BlueprintPure, Category = "Enemy|Deflect")
	bool IsDeflectWindowActive() const;

	UFUNCTION(BlueprintCallable, Category = "Enemy|Deflect")
	void ClearDeflectSucceeded();

	UFUNCTION(BlueprintPure, Category = "Enemy|Deflect")
	bool IsDeflectSucceeded() const;

	UFUNCTION(BlueprintCallable, Category = "Enemy|Deflect")
	void NotifyDeflectSuccess(AActor* Instigator);

	/** 反击条打空时直接触发：播敌人弹开蒙太奇并通知玩家被弹开（不需要 StateTree 事件）。 */
	UFUNCTION(BlueprintCallable, Category = "Enemy|Deflect")
	void TriggerDeflect(AActor* Instigator);

	/** 是否正在播放弹开蒙太奇（用于让所有战斗任务像受击一样自动中止）。 */
	UFUNCTION(BlueprintPure, Category = "Enemy|Deflect")
	bool IsDeflecting() const { return bIsDeflecting; }

	UFUNCTION(BlueprintCallable, Category = "Enemy|Deflect")
	void SetDeflecting(bool bInDeflecting) { bIsDeflecting = bInDeflecting; }

	// ---- 阶段 ----
	UFUNCTION(BlueprintPure, Category = "Enemy|Phase")
	int32 GetCurrentPhase() const { return CurrentPhaseIndex; }

	UFUNCTION(BlueprintCallable, Category = "Enemy|Phase")
	void SetPhase(int32 NewPhase);

	// ---- 招式池 ----
	UFUNCTION(BlueprintPure, Category = "Enemy|Moves")
	bool HasMoveSet() const;

	/** 组装一条完整近战链（起手式 + 中间招式段 + 收尾式），按各自距离/权重选择；无可用招式返回 false。 */
	bool PickCombo(TArray<URHEnemyMoveDefinition*>& OutChain) const;

	/** 按当前距离区间 + 权重选一个起手式（禁止特殊起手）。 */
	URHEnemyMoveDefinition* PickOpener(const TArray<FRHEnemyOpenerEntry>& Openers) const;

	/** 按当前距离区间 + 权重选一个收尾式。 */
	URHEnemyMoveDefinition* PickFinisher(const TArray<FRHEnemyFinisherEntry>& Finishers) const;

	/** 远程固定普攻：从远程招式数组随机选一个（等权重）。 */
	URHEnemyMoveDefinition* PickRemoteMove(const TArray<TObjectPtr<URHEnemyMoveDefinition>>& RemoteMoves) const;

	UFUNCTION(BlueprintPure, Category = "Enemy|Moves")
	bool IsSpecialAvailable() const;

	UFUNCTION(BlueprintCallable, Category = "Enemy|Moves")
	URHEnemyActionDefinition* PickSpecial();

	/** 从指定中间招式段池选一条段（按距离 + 权重），写入 OutChain；无可用返回 false。 */
	bool PickComboFromSet(const TArray<FRHEnemyComboChain>& MoveSet, TArray<URHEnemyMoveDefinition*>& OutChain) const;

	bool IsSpecialAvailableFromSet(const TArray<FRHEnemySpecialEntry>& Specials) const;

	URHEnemyActionDefinition* PickSpecialFromSet(const TArray<FRHEnemySpecialEntry>& Specials);

	// ---- 远程冷却 ----
	/** 远程冷却是否已结束（可触发远程）。 */
	UFUNCTION(BlueprintPure, Category = "Enemy|Moves")
	bool CanUseRemote() const { return RemoteCooldownRemaining <= 0.f; }

	/** 触发一次远程：按 CurrentConfig.RemoteCooldownSeconds 启动冷却。 */
	UFUNCTION(BlueprintCallable, Category = "Enemy|Moves")
	void NotifyRemoteUsed();

	UFUNCTION(BlueprintPure, Category = "Enemy|Moves")
	float GetRemoteCooldownRemaining() const { return RemoteCooldownRemaining; }

	// ---- 查询 ----
	UFUNCTION(BlueprintPure, Category = "Enemy|AI")
	const FRHPlayerCombatSnapshot& GetSnapshot() const { return Snapshot; }

	UFUNCTION(BlueprintPure, Category = "Enemy|AI")
	AActor* GetPlayerTarget() const { return PlayerTarget; }

	/** 把外部事件（受击/破防/死亡/取消窗口等）发给状态树（经 AIController 的 StateTree 组件）。 */
	UFUNCTION(BlueprintCallable, Category = "Enemy|AI")
	void SendAIEvent(FGameplayTag Tag);

	/** Dodge cooldown helpers for the state tree dodge task. */
	bool CanDodge() const;
	void NotifyDodgeStarted();

	/** 是否正在执行动作（战斗组件忙），根转移防重入用（Attack 转移取反）。 */
	UFUNCTION(BlueprintPure, Category = "Enemy|AI")
	bool IsAttacking() const;

	/** 屏幕 + 日志双输出调试信息（受 bShowDebugPrints 控制）。 */
	UFUNCTION(BlueprintCallable, Category = "Enemy|AI|Debug")
	void DebugPrint(const FString& Message) const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|AI|Debug")
	bool bShowDebugPrints = false;

	const FRHEnemyRuntimeConfig& GetCurrentConfig() const { return CurrentConfig; }

	UFUNCTION(BlueprintPure, Category = "Enemy|AI")
	URHEnemyCombatComponent* GetEnemyCombatComponent() const;

	UFUNCTION(BlueprintPure, Category = "Enemy|AI")
	UKnsAbilitySystemComponent* GetEnemyASC() const;

	UFUNCTION(BlueprintPure, Category = "Enemy|AI")
	FGameplayTag GetPlayerIntent() const { return Snapshot.PlayerIntent; }

	/** 每帧刷新玩家快照（StateTree 的 Enemy Context Evaluator 也会驱动它）。 */
	UFUNCTION(BlueprintCallable, Category = "Enemy|AI")
	void RefreshSnapshot();

	/** 角色级旋转到玩家：默认 true；Break/WaitRecover/Phase/Death 等任务可置 false。 */
	UFUNCTION(BlueprintCallable, Category = "Enemy|AI")
	void SetRotateToPlayer(bool bRotate);

	UFUNCTION(BlueprintPure, Category = "Enemy|AI")
	bool ShouldRotateToPlayer() const { return bRotateToPlayer; }

	/** 破防中受击 → 直接进入处刑（跳过范围/按键检测，保留武器处决蒙太奇可用性检查）。由受击管线调用。 */
	UFUNCTION(BlueprintCallable, Category = "Enemy|AI")
	void RequestExecutionFromHit();

	/** 本次受击是否就是触发破防的那一下（EnterBreak 同帧）。用于区分"刚进破防"与"破防后再次受击"。 */
	UFUNCTION(BlueprintPure, Category = "Enemy|Resonance")
	bool WasJustBroken() const;

protected:
	UFUNCTION()
	void HandlePlayerHitApplied(AActor* Target, const FRHHitData& HitData);

	UFUNCTION()
	void HandleActorDied(AActor* Actor);

	void Initialize();
	void RefreshPlayerReference();
	void TickCounterBar(float DeltaTime);
	void TickRotateToPlayer(float DeltaTime);
	void TickCounterAttack(float DeltaTime);
	void TickResonance(float DeltaTime);
	/** 破防（处决等待）状态维护：无蒙太奇播放时重播倒地 start 段（受击打断后自动回倒地）。 */
	void TickBreakState(float DeltaTime);
	void MaybeRequestExecution();
	/** 处决执行体（定位玩家 + 播双方处决蒙太奇），由 MaybeRequestExecution / RequestExecutionFromHit 调用。 */
	void StartPlayerExecution();
	void SetStateTreeLogicEnabled(bool bEnabled);
	void EnterBreak();
	void ExitBreak(bool bStartGetupInvincible);
	void HandleBreakDepleted();
	void EndGetupInvincible();
	void ApplyCurrentConfig();
	void InitAttributes();

	UPROPERTY(Transient)
	FRHPlayerCombatSnapshot Snapshot;

	UPROPERTY(Transient)
	FRHEnemyRuntimeConfig CurrentConfig;

	UPROPERTY(Transient)
	float CounterBarValue = 0.f;

	UPROPERTY(Transient)
	bool bResonanceBroken = false;

	/** EnterBreak 时的帧号：同帧的受击 = 触发破防的那一下（只进破防不处决），跨帧的受击 = 破防后再次受击（直接处刑）。 */
	uint32 BreakTriggerFrame = 0;

	UPROPERTY(Transient)
	bool bIsDowned = false;

	UPROPERTY(Transient)
	bool bDeflectWindowActive = false;

	UPROPERTY(Transient)
	bool bDeflectSucceeded = false;

	/** 当前阶段号（1-based：1 = 基础，Phases[0] = 2、Phases[1] = 3 ……）。 */
	UPROPERTY(Transient)
	int32 CurrentPhaseIndex = 1;

	UPROPERTY(Transient)
	float SpecialCooldownRemaining = 0.f;

	/** 远程冷却剩余秒数（<=0 可触发远程；Tick 内自然衰减）。 */
	UPROPERTY(Transient)
	float RemoteCooldownRemaining = 0.f;

	UPROPERTY(Transient)
	float LastPlayerAttackTime = -100.f;

	UPROPERTY(Transient)
	float LastDodgeTime = -1.f;

	UPROPERTY(Transient)
	bool bWasPlayerAttacking = false;

	UPROPERTY(Transient)
	bool bWasPlayerMeleeIntent = false;

	UPROPERTY(Transient)
	bool bExecutionDamageApplied = false;

	/** 处决进行中：共振回 0 时不走 HandleBreakDepleted（不播 down end）。 */
	UPROPERTY(Transient)
	bool bExecutionInProgress = false;

	/** 处决中被打空血：等处决动画自然播完（FinishExecution）再销毁，不立即 Destroy。 */
	UPROPERTY(Transient)
	bool bPendingDeathAfterExecution = false;

	UPROPERTY(Transient)
	bool bIsDeflecting = false;

	UPROPERTY(Transient)
	bool bRotateToPlayer = true;

	/** 本次破防是否已发起处决（防止每帧重复触发）。 */
	UPROPERTY(Transient)
	bool bExecutionEventSent = false;

	UPROPERTY(Transient)
	bool bAwaitingCounterAttack = false;

	UPROPERTY(Transient)
	bool bCounterAttackTreeRestarted = false;

	UPROPERTY(Transient)
	float CounterAttackDelayRemaining = 0.f;

	UPROPERTY(Transient)
	TObjectPtr<AActor> PlayerTarget;

	UPROPERTY(Transient)
	TObjectPtr<UKnsAbilitySystemComponent> PlayerASC;

	UPROPERTY(Transient)
	FTimerHandle GetupInvincibleTimerHandle;

	FGameplayTag TagPlayerBusy;
	FGameplayTag TagPlayerAttacking;
	FGameplayTag TagPlayerSkill;
	FGameplayTag TagPlayerLoad;
	FGameplayTag TagPlayerToss;
	FGameplayTag TagPlayerDodge;
	FGameplayTag TagPlayerBlock;
	FGameplayTag TagPlayerParry;
	FGameplayTag TagPlayerGuarding;
	FGameplayTag TagPlayerHealing;
	FGameplayTag TagIntentMelee;
	FGameplayTag TagIntentRanged;
	FGameplayTag TagIntentSkill;
	FGameplayTag TagDeflect;
	FGameplayTag TagExecution;
	FGameplayTag TagCounterAttack;
	FGameplayTag TagCancel;
	FGameplayTag TagStaggered;
	FGameplayTag TagInvincible;

	bool bInitialized = false;
	bool bPlayerCombatBound = false;
	bool bAttributesInitialized = false;
	double LastDebugLogTime = -1.0;
};
