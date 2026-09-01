#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Demo/Combat/KnsHitReactionSettingsDataAsset.h"
#include "Demo/Combat/KnsHitReactionThresholdDataAsset.h"
#include "Demo/Combat/KnsHitReactionTypes.h"
#include "Demo/Combat/KnsMoveDefinition.h"
#include "Demo/Combat/RHDefensiveCameraShakeDefinition.h"
#include "Demo/Combat/RHHitFeedbackDefinition.h"
#include "Demo/Combat/RHHitData.h"
#include "Demo/Onom/RHOnomActionDefinition.h"
#include "Demo/Onom/RHOnomTypes.h"
#include "Engine/TimerHandle.h"
#include "Engine/EngineTypes.h"
#include "GameplayTagContainer.h"
#include "Materials/MaterialInterface.h"
#include "KnsCombatComponent.generated.h"

class UKnsAbilitySystemComponent;
class UKnsCombatContextComponent;
class URHOnomComponent;
class URHWeaponDefinition;
class AWeaponBase;
class UNiagaraComponent;
class UNiagaraSystem;
class UKnsHitStopSettingsDataAsset;
class APlayerController;
class UAnimInstance;
class UAnimMontage;
class UBoxComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FKnsHitLanded, UKnsMoveDefinition*, Move, AActor*, TargetActor, FVector, HitLocation, FGameplayTag, HitboxTag);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FKnsHitStopTriggered, int32, Level, float, Duration, float, CameraShakeStrength);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FKnsRunExhausted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FKnsHitReceived, EKnsHitDirection, Direction, EKnsHitReactionStrength, Strength, int32, AttackPoiseLevel, int32, CurrentPoiseLevel);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FKnsHitReactionEnded);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FRHPlayerActionCast, URHOnomActionDefinition*, Action, const FRHOnomConsumptionData&, Consumption);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FRHPlayerHitApplied, AActor*, Target, const FRHHitData&, HitData);

/** 一次动作施放的命中结算上下文：共鸣增幅、打折与轰鸣格数（首次命中一次性）。 */
struct FRHOnomActionCastContext
{
	URHOnomActionDefinition* Action = nullptr;
	FRHOnomConsumptionData Consumption;
	FRHOnomAmplification Amplification;
	float DiscountMultiplier = 1.f;
};

/** 当前动作状态：整个动作期间有效（多段命中都算动作命中），与一次性轰鸣上下文分离。 */
struct FRHOnomActionState
{
	URHOnomActionDefinition* Action = nullptr;
	FRHOnomResolvedAction Resolved;
	/** 本次动作实际消耗的音形数目（技能命中反馈按此分档）。 */
	int32 ConsumedCount = 0;
	/** 本次动作实际消耗的 |和值|（技能命中反馈按此分档）。 */
	int32 ConsumedAbsoluteSum = 0;
};

USTRUCT(BlueprintType)
struct FCombatAttributeInitializer
{
	GENERATED_BODY()

	// 是否在 BeginPlay 时应用初始属性
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Init", meta = (ToolTip = "是否在 BeginPlay 时应用初始属性"))
	bool bApplyOnBeginPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Init", meta = (ClampMin = "0.0", ToolTip = "生命上限"))
	float MaxHealth = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Init", meta = (ClampMin = "0.0", ToolTip = "当前生命值"))
	float Health = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Init", meta = (ClampMin = "0.0", ToolTip = "体力上限"))
	float MaxStamina = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Init", meta = (ClampMin = "0.0", ToolTip = "当前体力"))
	float Stamina = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Init", meta = (ClampMin = "0.0", ToolTip = "攻击力"))
	float AttackPower = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Init", meta = (ClampMin = "0.0", ClampMax = "1.0", ToolTip = "暴击率，0~1"))
	float CritRate = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Init", meta = (ClampMin = "0.0", ClampMax = "1.0", ToolTip = "抗性，0~1"))
	float Resistance = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Init", meta = (ClampMin = "0.0", ToolTip = "防御力"))
	float Defense = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Init", meta = (ClampMin = "0.0", ClampMax = "1.0", ToolTip = "减伤系数，0~1"))
	float DamageReduction = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Init", meta = (ClampMin = "0.0", ToolTip = "韧性上限"))
	float MaxPoise = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Init", meta = (ClampMin = "0.0", ToolTip = "当前韧性"))
	float Poise = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Init", meta = (ClampMin = "0.0", ToolTip = "共振值上限"))
	float MaxResonance = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Init", meta = (ClampMin = "0.0", ToolTip = "当前共振值"))
	float Resonance = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Init", meta = (ClampMin = "0.0", ToolTip = "Onom 上限（仅玩家使用）"))
	float MaxOnom = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Init", meta = (ClampMin = "0.0", ToolTip = "当前 Onom 数量（仅玩家使用）"))
	float Onom = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Init", meta = (ClampMin = "0.0", ToolTip = "Focus 上限（仅玩家使用）"))
	float MaxFocus = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Init", meta = (ClampMin = "0.0", ToolTip = "当前 Focus 充能（仅玩家使用）"))
	float Focus = 0.f;
};

UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class DEMO_API UKnsCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UKnsCombatComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// 当前武器战斗数据 DA（武器 Actor 类/主手 Socket 名也由此 DA 提供）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RH|Weapon")
	TObjectPtr<URHWeaponDefinition> WeaponDefinition;

	// 已生成的武器 Actor
	UPROPERTY(BlueprintReadOnly, Transient, Category = "Weapon", meta = (ToolTip = "已生成的武器 Actor"))
	TObjectPtr<AWeaponBase> SpawnedWeapon;

	/** 命中/防御反馈（音效+特效）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RH|Feedback")
	TObjectPtr<URHHitFeedbackDefinition> HitFeedbackDefinition;

	// 初始属性配置，BeginPlay 时写入 Common AS 和 Player AS
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attributes", meta = (ToolTip = "初始属性配置，BeginPlay 时写入 Common AS 和 Player AS"))
	FCombatAttributeInitializer AttributeInitializer;

	// 翻滚体力消耗
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stamina", meta = (ClampMin = "0.0", ToolTip = "翻滚体力消耗"))
	float RollStaminaCost = 25.f;

	// 防御体力消耗
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stamina", meta = (ClampMin = "0.0", ToolTip = "防御体力消耗"))
	float DefendStaminaCost = 15.f;

	// 跑步每秒体力消耗
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stamina", meta = (ClampMin = "0.0", ToolTip = "跑步每秒体力消耗"))
	float RunStaminaCostPerSecond = 10.f;

	// 跑步耐力耗尽时触发
	UPROPERTY(BlueprintAssignable, Category = "Stamina|Events", meta = (ToolTip = "跑步耐力耗尽时触发"))
	FKnsRunExhausted OnRunExhausted;

	/** 受击播放阈值（独立 DA，韧性差值 → 轻/中/重/倒地）。受击蒙太奇由武器 DA 的 HitReaction 按当前武器查询。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitReaction", meta = (ToolTip = "受击播放阈值配置"))
	TObjectPtr<UKnsHitReactionThresholdDataAsset> HitReactionThresholds;

	// 兼容保留：不再参与任何屏幕打印
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitReaction|Debug", meta = (ToolTip = "兼容保留：不再参与任何屏幕打印"))
	bool bShowPoiseLevel = true;

	/** 调试用：勾选后，命中框开启（BeginHitbox，含相杀等以 hitbox 判定的事件）期间把武器 HitboxBox 的 HiddenInGame 关闭，让命中框在游戏中可见；命中框关闭（EndHitbox）时恢复隐藏。玩家/敌人共用（基类）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Hitbox|Debug", meta = (ToolTip = "调试用：命中框开启期间让武器 HitboxBox 在游戏中可见"))
	bool bShowHitboxDebug = false;

	// 兼容保留：不再参与任何屏幕打印
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitReaction|Debug", meta = (ToolTip = "兼容保留：不再参与任何屏幕打印"))
	bool bShowHitReactionDebug = true;

	// 兼容保留：不再绘制 Hitbox 胶囊
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitReaction|Debug", meta = (ToolTip = "兼容保留：不再绘制 Hitbox 胶囊"))
	bool bDrawHitboxDebug = true;

	// 一次受击打断判定后触发
	UPROPERTY(BlueprintAssignable, Category = "HitReaction|Events", meta = (ToolTip = "一次受击打断判定后触发"))
	FKnsHitReceived OnHitReceived;

	// 受击蒙太奇播完时触发
	UPROPERTY(BlueprintAssignable, Category = "HitReaction|Events", meta = (ToolTip = "受击蒙太奇播完时触发"))
	FKnsHitReactionEnded OnHitReactionEnded;

	// 生成武器并挂到角色 Mesh Socket
	UFUNCTION(BlueprintCallable, Category = "Weapon", meta = (ToolTip = "生成武器并挂到角色 Mesh Socket"))
	AWeaponBase* SpawnAndAttachWeapon();

	// 销毁当前生成的武器
	UFUNCTION(BlueprintCallable, Category = "Weapon", meta = (ToolTip = "销毁当前生成的武器"))
	void DestroySpawnedWeapon();

	/** Switch weapon: destroy current actor, respawn with the weapon class/socket from the data asset and swap combat data. Default initial weapon untouched. */
	UFUNCTION(BlueprintCallable, Category = "Weapon", meta = (ToolTip = "Switch weapon by weapon data asset（武器类/主手 Socket 从 DA 取）"))
	bool SwitchWeapon(URHWeaponDefinition* NewWeaponDefinition);

	// 按 AttributeInitializer 初始化属性
	UFUNCTION(BlueprintCallable, Category = "Attributes", meta = (ToolTip = "按 AttributeInitializer 初始化属性"))
	void ApplyAttributeInitializer();

	// 尝试消耗翻滚体力，成功返回 true
	UFUNCTION(BlueprintCallable, Category = "Stamina", meta = (ToolTip = "尝试消耗翻滚体力，成功返回 true"))
	bool TryConsumeRollStamina();

	// 尝试消耗防御体力，成功返回 true
	UFUNCTION(BlueprintCallable, Category = "Stamina", meta = (ToolTip = "尝试消耗防御体力，成功返回 true"))
	bool TryConsumeDefendStamina();

	// 设置是否处于跑步状态，跑步时每帧按 RunStaminaCostPerSecond 扣体力
	UFUNCTION(BlueprintCallable, Category = "Stamina", meta = (ToolTip = "设置是否处于跑步状态，跑步时每帧按 RunStaminaCostPerSecond 扣体力"))
	void SetRunning(bool bInRunning);

	// 当前是否处于跑步状态
	UFUNCTION(BlueprintPure, Category = "Stamina", meta = (ToolTip = "当前是否处于跑步状态"))
	bool IsRunning() const;

	// 处理一次受击，攻击韧性大于当前招式韧性才会打断
	UFUNCTION(BlueprintCallable, Category = "HitReaction", meta = (ToolTip = "处理一次受击，攻击韧性大于当前招式韧性才会打断"))
	void HandleIncomingHit(EKnsHitDirection Direction, int32 AttackPoiseLevel, AActor* AttackSource = nullptr);

	/** 统一命中管线入口：由来源方向计算受击方向后处理受击。 */
	UFUNCTION(BlueprintCallable, Category = "HitReaction", meta = (ToolTip = "Handle incoming hit from a source actor"))
	void HandleIncomingHitFrom(AActor* Source, int32 AttackPoiseLevel);

	/** 按命中点计算受击方向（命中点无效时回落到来源位置）。 */
	UFUNCTION(BlueprintCallable, Category = "HitReaction", meta = (ToolTip = "Handle incoming hit by hit location"))
	void HandleIncomingHitAt(FVector HitLocation, AActor* FallbackSource, int32 AttackPoiseLevel);

	/** 当前是否在播受击动画（硬直中），供 AI 判断能否重开动作。 */
	UFUNCTION(BlueprintPure, Category = "HitReaction")
	bool IsHitReactionPlaying() const { return CurrentReactionMontage != nullptr; }

	/**
	 * 受击/硬直动画是否仍在播放（含被替换/Stop 的 blend out 间隙）。
	 * CurrentReactionMontage 在 Montage_Stop 时就被清空，但动画实例上旧蒙太奇还在 blend out；
	 * 连续命中替换瞬间指针也会短暂为空。AI 恢复判定应以此为准——蒙太奇真正播完才允许重选动作。
	 */
	bool IsReactionAnimationActive() const;

	/** 当前招式的减伤系数（执行中被命中时生效）。 */
	UFUNCTION(BlueprintPure, Category = "HitReaction", meta = (ToolTip = "Current move damage reduction"))
	float GetCurrentResistance() const;

	/** 倒地起身：移除倒地无敌与倒地 tag（起身 Notify 调用）。 */
	UFUNCTION(BlueprintCallable, Category = "HitReaction", meta = (ToolTip = "End knockdown invincibility"))
	void EndKnockdown();

	// 调试用：直接触发一次受击，可填方向和攻击韧性
	UFUNCTION(BlueprintCallable, Category = "HitReaction|Debug", meta = (DevelopmentOnly, ToolTip = "调试用：直接触发一次受击，可填方向和攻击韧性"))
	void DebugTriggerHitReaction(EKnsHitDirection Direction, int32 AttackPoiseLevel);

	// 卡肉等级对应的全局配置
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitStop", meta = (ToolTip = "卡肉等级对应的全局配置"))
	TObjectPtr<UKnsHitStopSettingsDataAsset> HitStopSettings;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RH|Defensive", meta = (ToolTip = "Defensive shake DA: guard hit / big break / parry, no montage rate change"))
	TObjectPtr<URHDefensiveCameraShakeDefinition> DefensiveCameraShakeDefinition;

	// 一次命中触发
	UPROPERTY(BlueprintAssignable, Category = "Combat|Events", meta = (ToolTip = "一次命中触发"))
	FKnsHitLanded OnHitLanded;

	UPROPERTY(BlueprintAssignable, Category = "RH|Events")
	FRHPlayerHitApplied OnHitApplied;

	UPROPERTY(BlueprintAssignable, Category = "RH|Events")
	FRHPlayerActionCast OnActionCast;

	// 触发卡肉时触发，供蓝图接震屏/音效
	UPROPERTY(BlueprintAssignable, Category = "Combat|Events", meta = (ToolTip = "触发卡肉时触发，供蓝图接震屏/音效"))
	FKnsHitStopTriggered OnHitStopTriggered;

	// 由 Hitbox NotifyState 调用，开启一段命中框
	UFUNCTION(BlueprintCallable, Category = "Combat|Hitbox", meta = (ToolTip = "由 Hitbox NotifyState 调用，开启一段命中框"))
	void BeginHitbox(FGameplayTag HitboxTag);

	// 由 Hitbox NotifyState 调用，关闭一段命中框
	UFUNCTION(BlueprintCallable, Category = "Combat|Hitbox", meta = (ToolTip = "由 Hitbox NotifyState 调用，关闭一段命中框"))
	void EndHitbox(FGameplayTag HitboxTag);

	/** 相杀结算（被对方相杀窗口命中时调用）：本方命中框完成判定（记录该武器并关闭命中框）。 */
	UFUNCTION(BlueprintCallable, Category = "RH|Combat")
	void ResolveWeaponClash(AActor* OtherWeapon);

	/** 当前武器命中框是否开着（相杀判定用）。 */
	UFUNCTION(BlueprintPure, Category = "RH|Combat")
	bool IsHitboxActive() const { return ActiveHitboxTag.IsValid(); }

	/**
	 * 是否为可相杀命中框（相杀判定用）：默认 = 当前武器命中框开启（IsHitboxActive）。
	 * 木桩等无武器自定义判定框可覆写为自身判定框状态（如 RHTestDummy 的 ProbeBox 激活即视为可相杀）。
	 */
	UFUNCTION(BlueprintPure, Category = "RH|Combat")
	virtual bool IsClashableHitboxActive() const { return IsHitboxActive(); }

	/**
	 * 受击是否应跳过受击动画（OnHitReceived 已广播，用于转入其它反应）。
	 * 敌人破防（处决等待）中受击 → 覆写返回 true：不再播受击蒙太奇，由 HandleEnemyHitReceived 直接转处刑。
	 */
	virtual bool ShouldSkipHitReaction() const { return false; }

	/** 本段命中框是否已对目标结算过（普通命中或相杀，用于防重复结算）。 */
	UFUNCTION(BlueprintPure, Category = "RH|Combat")
	bool HasResolvedHitboxTarget(AActor* Target) const { return Target && HitActorsThisHitbox.Contains(Target); }

	// 命中检测回调，会应用伤害/削韧并触发卡肉
	UFUNCTION(BlueprintCallable, Category = "Combat", meta = (ToolTip = "命中检测回调，会应用伤害/削韧并触发卡肉"))
	virtual bool ReportHit(AActor* TargetActor, FVector HitLocation, FGameplayTag HitboxTag);

	UFUNCTION(BlueprintCallable, Category = "RH|Combat")
	bool ApplyHitToTarget(AActor* Target, const FRHHitData& HitData);

	/** 手动标记受击方向：HitReaction.Direction.{F,L,R,B}（兼容 Front/Back/Left/Right）→ EKnsHitDirection。 */
	static bool ResolveHitDirectionTag(const FGameplayTag& Tag, EKnsHitDirection& OutDirection);

	UFUNCTION(BlueprintCallable, Category = "RH|Combat")
	void HandleNormalGuardHit();

	UFUNCTION(BlueprintCallable, Category = "RH|Combat")
	void HandlePerfectGuardHit();

	UFUNCTION(BlueprintCallable, Category = "RH|Combat")
	void HandleDamageTaken(AActor* Instigator = nullptr);

	/** 闪避成功：默认执行 Dodge Core 效果（无配置时回落共鸣时间奖励）；玩家组件覆写后追加 SFX/材质/相机效果。 */
	UFUNCTION(BlueprintCallable, Category = "RH|Combat")
	virtual void HandleDodgeSuccess();

	/** 依次执行一组效果（共鸣时长/音形增减/触发 just load/触发弹反）。 */
	UFUNCTION(BlueprintCallable, Category = "RH|Onom")
	void ApplyOnomEffects(const TArray<FRHOnomEffect>& Effects);

	/** 当前普攻/招式对命中 Onom 规则的覆盖（默认无；玩家组件按当前 Move 的 bOverrideHitOnom 返回）。 */
	virtual bool GetHitOnomRuleOverride(FRHOnomSourceRule& OutRule) const { return false; }

	/** 触发 just load：无蒙太奇直接装填（无灰）/抛弹（有灰），播 load 音效；bPlayVFX 时由子类播 parry 同款 VFX。 */
	UFUNCTION(BlueprintCallable, Category = "RH|Onom")
	virtual void ExecuteJustLoad(bool bPlayVFX);

	/** 触发弹反成功效果（基类空实现，玩家组件覆写为开取消窗口等）。 */
	UFUNCTION(BlueprintCallable, Category = "RH|Onom")
	virtual void TriggerParryEffect();

	/** 打开/关闭普通防御状态（RH Guard Window ANS 调用，只管理 Status.Guarding；完美窗口由 RH Parry Window/抛弹单独开）。 */
	UFUNCTION(BlueprintCallable, Category = "RH|Guard")
	void SetGuarding(bool bGuarding);

	UFUNCTION(BlueprintPure, Category = "RH|Guard")
	bool IsGuarding() const;

	/** 弹开/防御破防受击：播防御破防蒙太奇（F/L/R/B section），并打断当前动作。 */
	UFUNCTION(BlueprintCallable, Category = "RH|Guard")
	void PlayDefensiveBreakReaction(AActor* AttackSource = nullptr);

	/** 防御态移动速度（按下防御时生效，结束/被打断恢复 DefaultMaxWalkSpeed）。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RH|Guard", meta = (ClampMin = "0"))
	float GuardWalkSpeed = 200.f;

	/** BeginPlay 时记录的默认移动速度，防御结束恢复用。 */
	UPROPERTY(Transient)
	float DefaultMaxWalkSpeed = 600.f;

	/** 当前是否处于完美防御窗口（含抛弹窗口）。 */
	UFUNCTION(BlueprintPure, Category = "RH|Guard")
	bool IsPerfectGuardWindowActive() const;

	/** 无敌帧开关（闪避等 ANS 调用）：切 Status.Invincible。 */
	UFUNCTION(BlueprintCallable, Category = "RH|Combat")
	void SetInvincible(bool bInvincible);

	/** 当前动作施放的极性（拖尾/闪光按此取配置）。 */
	UFUNCTION(BlueprintPure, Category = "RH|Action")
	ERHOnomPolarity GetCurrentCastPolarity() const;

	/** 本次普攻命中会获得的 Onom 极性（按 AttackHitRule 映射，供命中反馈选档）。 */
	UFUNCTION(BlueprintPure, Category = "RH|Action")
	ERHOnomPolarity GetCurrentHitOnomPolarity() const;

	/** 当前动作实际消耗的音形数目。 */
	UFUNCTION(BlueprintPure, Category = "RH|Action")
	int32 GetCurrentActionConsumedCount() const;

	/** 当前动作实际消耗的 |和值|。 */
	UFUNCTION(BlueprintPure, Category = "RH|Action")
	int32 GetCurrentActionAbsoluteSum() const;

	/** 完美防御窗口开启时给角色 mesh 上的覆层材质（窗口关闭/超时后自动下掉）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RH|Guard")
	TObjectPtr<UMaterialInterface> ParryOverlayMaterial;

	/** 开启一段完美防御窗口（抛弹/GP 等用）。 */
	UFUNCTION(BlueprintCallable, Category = "RH|Guard")
	void OpenPerfectGuardWindow(float Seconds);

	/** 立即关闭完美防御窗口并清掉计时器（RH Parry Window ANS 结束/打断时调用）。 */
	UFUNCTION(BlueprintCallable, Category = "RH|Guard")
	void ClosePerfectGuardWindow();

	/** 受击时结算防御结果：完美窗口=小调，普通防御=破碎（含吞格与大破防）。 */
	UFUNCTION(BlueprintCallable, Category = "RH|Guard")
	ERHOnomGuardOutcome ResolveGuardHit(bool bPerfect);

	/** 大破防：固定伤害 + 硬直，不吃来袭伤害。 */
	UFUNCTION(BlueprintCallable, Category = "RH|Guard")
	void ApplyBigBreak(AActor* AttackSource = nullptr);

	UFUNCTION(BlueprintCallable, Category = "RH|Combat")
	bool TryStartAction(URHOnomActionDefinition* Action);

	/** 激活武器拖尾（战技/敌人特殊招式施放反馈）。 */
	UFUNCTION(BlueprintCallable, Category = "RH|Feedback")
	void ActivateCastTrail();

	/** 关闭武器拖尾（动作结束/取消/被打断）。 */
	UFUNCTION(BlueprintCallable, Category = "RH|Feedback")
	void DeactivateCastTrail();

	UFUNCTION(BlueprintPure, Category = "RH|Combat")
	TArray<URHOnomActionDefinition*> GetEligibleActions(const TArray<URHOnomActionDefinition*>& Actions);

	// 平滑旋转到目标朝向，InterpSpeed 为插值速度，0 表示瞬转
	UFUNCTION(BlueprintCallable, Category = "Combat|Rotation", meta = (ToolTip = "平滑旋转到目标朝向，InterpSpeed 为插值速度，0 表示瞬转"))
	void StartRotateToMoveInput(const FRotator& InTargetRotation, float InInterpSpeed);

	UFUNCTION(BlueprintCallable, Category = "RH|Defensive")
	void PlayDefensiveCameraShake(ERHDefensiveShakeType Type);

	/** 施放闪光：mesh 附属 Niagara 组件，每次施放播一次（资产由策划配）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RH|Feedback")
	TObjectPtr<UNiagaraSystem> SpecialCastFlash;

	/** 施放闪光挂点 Socket（默认 root）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RH|Feedback")
	FName CastFlashSocket = TEXT("root");

	UFUNCTION(BlueprintCallable, Category = "RH|Feedback")
	void PlayCastFlash();

	// 由 TouchCondition NotifyState 调用，开启/关闭实体接触检测
	UFUNCTION(BlueprintCallable, Category = "Combat|Condition", meta = (ToolTip = "由 TouchCondition NotifyState 调用，开启/关闭实体接触检测"))
	void SetTouchConditionEnabled(bool bEnabled);

	// 当前是否允许翻滚取消：根据 State.Player.Attacking 和 Combo.Cancel.Roll 两个 Tag 判断
	UFUNCTION(BlueprintPure, Category = "Combat|Cancel", meta = (ToolTip = "当前是否允许翻滚取消：根据 State.Player.Attacking 和 Combo.Cancel.Roll 两个 Tag 判断"))
	bool CanRollCancel() const;

protected:
	UFUNCTION()
	void HandlePerfectGuardWindowElapsed();

	/** 按窗口状态给角色 mesh 设置/移除 ParryOverlayMaterial（无引用或缺 mesh 时静默跳过）。 */
	void ApplyParryOverlay(bool bActive);

	UFUNCTION()
	void HandleBigBreakEnded();

	UFUNCTION()
	void HandleHitboxOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/** 补扫一个命中框 box：武器已与目标重叠时 BeginOverlap 不会再次触发，开启命中框时主动查询一次（主/副刀 box 共用）。 */
	void QueryHitboxOverlaps(UBoxComponent* HitboxBox);

	UFUNCTION()
	void HandleBodyHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	void PlayHitReaction(EKnsHitDirection Direction, EKnsHitReactionStrength Strength, int32 AttackPoiseLevel, int32 CurrentPoiseLevel, AActor* AttackSource = nullptr);
	void StopCurrentReactionMontage();

	UFUNCTION()
	void HandleReactionMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	void PlayHitStop(int32 Level);
	/** 共用震动出口：查配置后对给定玩家控制器播放 DynamicForceFeedback，时长缺省用 DefaultDuration。 */
	void PlayControllerRumble(APlayerController* PlayerController, int32 HitStopLevel, float DefaultDuration);
	void RestoreMontagePlayRate(UAnimInstance* AnimInstance, UAnimMontage* Montage);
	UKnsAbilitySystemComponent* GetAbilitySystemComponent() const;
	URHOnomComponent* GetOnomComponent() const;
	/** 当前武器绑定的受击蒙太奇 DA（无武器时为 nullptr，敌人受击走 OnHitReceived 自己的逻辑）。 */
	UKnsHitReactionSettingsDataAsset* GetHitReactionData() const;

	UPROPERTY(Transient)
	FGameplayTag ActiveHitboxTag;

	UPROPERTY(Transient)
	TArray<AActor*> HitActorsThisHitbox;

	UPROPERTY(Transient)
	FTimerHandle HitStopTimerHandle;

	UPROPERTY(Transient)
	float OriginalMontagePlayRate = 1.f;

	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> CurrentReactionMontage;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> CastFlashComponent;

	UPROPERTY(Transient)
	bool bIsRunning = false;

	UPROPERTY(Transient)
	FRotator RotationInterpTarget = FRotator::ZeroRotator;

	UPROPERTY(Transient)
	float RotationInterpSpeed = 0.f;

	UPROPERTY(Transient)
	bool bRotationInterpActive = false;

	UPROPERTY(Transient)
	bool bTouchConditionEnabled = false;

	UPROPERTY(Transient)
	TObjectPtr<UKnsCombatContextComponent> CombatContext;

	UPROPERTY(Transient)
	TObjectPtr<URHOnomComponent> CachedOnomComponent;

	UPROPERTY(Transient)
	bool bGuarding = false;

	UPROPERTY(Transient)
	bool bPerfectGuardWindowActive = false;

	UPROPERTY(Transient)
	FTimerHandle PerfectGuardTimerHandle;

	UPROPERTY(Transient)
	FTimerHandle BigBreakTimerHandle;

	/** 动作施放后、首次命中前保留的结算上下文。 */
	FRHOnomActionCastContext PendingActionCast;

	/** 当前动作（战技/音律武器/终结技，整个动作期间有效）。 */
	FRHOnomActionState CurrentActionState;

	/** 当前动作施放极性（由消耗和值决定，施放时写入）。 */
	UPROPERTY(Transient)
	ERHOnomPolarity CurrentCastPolarity = ERHOnomPolarity::None;

	UPROPERTY(Transient)
	ERHOnomPolarity CurrentHitOnomPolarity = ERHOnomPolarity::None;

};
