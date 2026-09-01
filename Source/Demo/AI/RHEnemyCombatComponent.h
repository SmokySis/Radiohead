#pragma once

#include "CoreMinimal.h"
#include "Demo/Combat/KnsCombatComponent.h"
#include "RHEnemyCombatComponent.generated.h"

class UAnimMontage;
class URHEnemyMoveDefinition;
class URHEnemyActionDefinition;
class ARHTestDummy;

/**
 * 敌人战斗组件：只负责“播蒙太奇 + 走现有 ANS 链路”。
 * 命中框/卡肉/受击/弹开窗口/GA 触发全部由蒙太奇上的 NotifyState 和现有设置 DA 驱动，代码不硬编码手感数值。
 */
UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class DEMO_API URHEnemyCombatComponent : public UKnsCombatComponent
{
	GENERATED_BODY()

public:
	URHEnemyCombatComponent();

	virtual void BeginPlay() override;
	virtual bool ReportHit(AActor* TargetActor, FVector HitLocation, FGameplayTag HitboxTag) override;

	/** 相杀判定查询：木桩判定框（ProbeBox）激活时视作命中框开启（可参与相杀）；其它敌人走武器 hitbox。 */
	virtual bool IsClashableHitboxActive() const override;

	/** 破防（处决等待）中受击：跳过受击动画，由 HandleEnemyHitReceived 直接转处刑。 */
	virtual bool ShouldSkipHitReaction() const override;

	/** 共振打满（破防）时的时间膨胀：参数可调。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Feel", meta = (ClampMin = "0"))
	float BreakTimeDilationScale = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Feel", meta = (ClampMin = "0"))
	float BreakTimeDilationDuration = 0.2f;

	UFUNCTION(BlueprintCallable, Category = "Enemy|Resonance")
	void TriggerBreakTimeDilation();

	/** 播放敌人普攻/连段（蒙太奇由 MoveDef 配置）。 */
	UFUNCTION(BlueprintCallable, Category = "Enemy|Action")
	bool PlayMove(URHEnemyMoveDefinition* Move);

	/** 播放敌人特殊招式（参考玩家 ActionDef，可选 GA 由 AN_CastEffect 触发）。 */
	UFUNCTION(BlueprintCallable, Category = "Enemy|Action")
	bool PlayAction(URHEnemyActionDefinition* Action);

	/** 播放一段状态级蒙太奇（格挡/弹开/倒地/起身/闪避等）。 */
	UFUNCTION(BlueprintCallable, Category = "Enemy|Action")
	bool PlayMontage(UAnimMontage* Montage, float PlayRate = 1.f);

	/** 播放死亡蒙太奇：播完进入 blend out 时冻结动画保持死亡姿势，避免死后 blend 回 Idle 站起。 */
	UFUNCTION(BlueprintCallable, Category = "Enemy|Action")
	bool PlayDeathMontage(UAnimMontage* Montage);

	/** 从指定 Section 播放蒙太奇（闪避 F/L/R/B、倒地 start/loop/end 等）。 */
	UFUNCTION(BlueprintCallable, Category = "Enemy|Action")
	bool PlayMontageSection(UAnimMontage* Montage, FName SectionName, float PlayRate = 1.f);

	UFUNCTION(BlueprintCallable, Category = "Enemy|Action")
	void StopCurrentMoveMontage();

	UFUNCTION(BlueprintCallable, Category = "Enemy|Action")
	void SetComboWindowOpen(bool bOpen);

	UFUNCTION(BlueprintPure, Category = "Enemy|Action")
	bool IsComboWindowOpen() const { return bEnemyComboWindowOpen; }

	UFUNCTION(BlueprintPure, Category = "Enemy|Action")
	bool IsBusy() const { return CurrentEnemyMove != nullptr || CurrentEnemyAction != nullptr; }

	UFUNCTION(BlueprintPure, Category = "Enemy|Action")
	bool IsMontagePlaying() const { return CurrentEnemyMontage != nullptr; }

	/** 玩家发起的处决：播敌人处决蒙太奇，播完自动调用 FinishExecution。 */
	UFUNCTION(BlueprintCallable, Category = "Enemy|Execution")
	void PlayExecutionMontage(UAnimMontage* Montage);

	/** 处决蒙太奇结束后接的 getup 蒙太奇。 */
	UFUNCTION(BlueprintCallable, Category = "Enemy|Execution")
	void PlayGetupMontage(UAnimMontage* Montage);

	/** 处决收尾：改播 DodgeMontage 的 B 方向（Back），播完再 FinishExecution。 */
	UFUNCTION(BlueprintCallable, Category = "Enemy|Execution")
	void PlayDodgeBackMontage(UAnimMontage* Montage);

	/** 弹开蒙太奇：不先 Stop 前一段，直接覆盖播放，避免回到 Idle。 */
	UFUNCTION(BlueprintCallable, Category = "Enemy|Deflect")
	void PlayDeflectMontage(UAnimMontage* Montage);

	UFUNCTION(BlueprintPure, Category = "Enemy|Action")
	URHEnemyMoveDefinition* GetCurrentEnemyMove() const { return CurrentEnemyMove; }

	UFUNCTION(BlueprintPure, Category = "Enemy|Action")
	URHEnemyActionDefinition* GetCurrentEnemyAction() const { return CurrentEnemyAction; }

	/** 上一段动作是否被受击打断（自然播完=false；被打断=true），供连段任务判断。 */
	UFUNCTION(BlueprintPure, Category = "Enemy|Action")
	bool WasLastActionInterrupted() const { return bActionInterrupted; }

	/** 由 AN_CastEffect 通知调用：触发当前特殊招式的 EffectAbility。 */
	UFUNCTION(BlueprintCallable, Category = "Enemy|Action")
	void HandleCastEffect();

protected:
	UFUNCTION()
	void HandleEnemyActionMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	UFUNCTION()
	void HandleDeathMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted);

	UFUNCTION()
	void HandleEnemyHitReceived(EKnsHitDirection Direction, EKnsHitReactionStrength Strength, int32 AttackPoiseLevel, int32 CurrentPoiseLevel);

	void PlayEffectAbility(TSubclassOf<UGameplayAbility> AbilityClass);

	UPROPERTY(Transient)
	TObjectPtr<URHEnemyMoveDefinition> CurrentEnemyMove;

	UPROPERTY(Transient)
	TObjectPtr<URHEnemyActionDefinition> CurrentEnemyAction;

	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> CurrentEnemyMontage;

	UPROPERTY(Transient)
	bool bActionInterrupted = false;

	UPROPERTY(Transient)
	bool bEnemyComboWindowOpen = false;

	UPROPERTY(Transient)
	bool bIsExecutionMontage = false;

	UPROPERTY(Transient)
	bool bIsGetupMontage = false;

	/** 当前播放的是否为死亡蒙太奇（blend out 时冻结动画用）。 */
	UPROPERTY(Transient)
	bool bIsDeathMontage = false;

	/** 正在播放的死亡蒙太奇：精确匹配 blend out 回调，防止被其它蒙太奇误判销毁。 */
	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> LastDeathMontage;

	/** 拥有者是否已死亡（Status.Dead）：死亡后跳过受击/停止逻辑，避免打断死亡蒙太奇。 */
	bool IsOwnerDead() const;

	UPROPERTY(Transient)
	FTimerHandle BreakTimeDilationTimerHandle;

};
