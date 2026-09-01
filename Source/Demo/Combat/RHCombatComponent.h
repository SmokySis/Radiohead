#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "Animation/AnimMontage.h"
#include "Core/CameraRigAsset.h"
#include "Demo/Combat/KnsCombatComponent.h"
#include "Demo/Combat/RHMoveDefinition.h"
#include "Demo/Onom/RHOnomActionDefinition.h"
#include "Materials/MaterialInterface.h"
#include "Sound/SoundBase.h"
#include "RHCombatComponent.generated.h"

/** 闪避成功反馈资产组：音效 / mesh 覆盖材质 / 相机 rig（时长共用 DodgeTimeDilationDuration）。 */
USTRUCT(BlueprintType)
struct FRHDodgeFeedback
{
	GENERATED_BODY()

	/** 闪避成功音效。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dodge")
	TObjectPtr<USoundBase> SFX;

	/** 闪避成功时给玩家 mesh 上的覆盖材质（持续 DodgeTimeDilationDuration 后下掉）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dodge")
	TObjectPtr<UMaterialInterface> OverlayMaterial;

	/** 闪避成功时激活的 persistent visual camera rig（持续 DodgeTimeDilationDuration 后关闭）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dodge")
	TObjectPtr<UCameraRigAsset> CameraRig;
};

UENUM(BlueprintType)
enum class ERHActionState : uint8
{
	Idle,
	Attacking,
	/** 战技 / 音律武器 / 终结技统一动作状态。 */
	Skill,
	Load,
	Toss,
	Dodge,
	/** 防御/格挡（HandleBlock 进入，持续到退出或破防）。 */
	Block,
	/** 弹反（武器 DA 的 DefensiveMontage L/R 段，窗口由蒙太奇上的 ANS 提供）。 */
	Parry,
	/** 受击/受伤动画（挂 Busy，无取消窗口；打断由受击蒙太奇上的 RH Cancel AN 控制）。 */
	HitReaction
};

/** 取消/插入类型：位掩码枚举，RH Cancel AN 可一次勾选多个。 */
UENUM(BlueprintType, meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class ERHCancelType : uint8
{
	None = 0 UMETA(DisplayName = "None", Hidden),
	Roll = 1 UMETA(DisplayName = "Roll"),
	Move = 2 UMETA(DisplayName = "Move"),
	Attack = 4 UMETA(DisplayName = "Attack"),
	Special = 8 UMETA(DisplayName = "Special"),
	Defensive = 16 UMETA(DisplayName = "Defensive"),
	/** Other：除当前动作类型外的全部取消类型（RH Cancel notify 选择此值时）。 */
	Other = 32 UMETA(DisplayName = "Other")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FRHActionLanded, URHMoveDefinition*, Move, AActor*, TargetActor, FVector, HitLocation, FGameplayTag, HitboxTag);

UCLASS()
class DEMO_API URHCombatComponent : public UKnsCombatComponent
{
	GENERATED_BODY()

public:
	URHCombatComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual bool ReportHit(AActor* TargetActor, FVector HitLocation, FGameplayTag HitboxTag) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RH|Health", meta = (ClampMin = "1"))
	float MaxHealth = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RH|Health", meta = (ClampMin = "0"))
	float Health = 100.f;

	UPROPERTY(BlueprintAssignable, Category = "RH|Events")
	FRHActionLanded OnRHActionLanded;

	UFUNCTION(BlueprintCallable, Category = "RH|Action")
	void HandleAttack();

	/** 释放武器战技（WeaponDefinition->Skills 数组下标）。 */
	UFUNCTION(BlueprintCallable, Category = "RH|Action")
	void HandleSkill(int32 SkillIndex);

	/** 释放音律武器（与战技同一套判定/消耗/充能/效果逻辑）。 */
	UFUNCTION(BlueprintCallable, Category = "RH|Action")
	void HandleRhythmWeapon(int32 WeaponIndex);

	/** 闪避：武器 DA 的闪避蒙太奇，按移动输入与面朝夹角选 F/L/R/B Section；无敌帧由蒙太奇上 ANS 挂。 */
	UFUNCTION(BlueprintCallable, Category = "RH|Action")
	void HandleDodge();

	/** 防御/格挡：true 进入（播武器 DA 的 DefensiveMontage 循环段，属于 Defensive 取消），false 退出。格挡期间不扣血。 */
	UFUNCTION(BlueprintCallable, Category = "RH|Action")
	void HandleBlock(bool bBlock);

	/** 立即退出防御/格挡（HandleBlock(false) 与破防时调用）。 */
	UFUNCTION(BlueprintCallable, Category = "RH|Action")
	void ExitBlock();

	UFUNCTION(BlueprintPure, Category = "RH|Action")
	bool IsBlocking() const;

	/** 弹反：播放武器 DA 的 DefensiveMontage（L/R 段）。属于 Defensive 取消——当前动作开了 Window.Cancel.Defensive 时可直接打断。 */
	UFUNCTION(BlueprintCallable, Category = "RH|Action")
	void HandleParry();

	/** 逆转逆装填（ReverseReload 类型武器的防御键）：需共鸣槽，挂 Parry 状态/tag 并播 DefensiveMontage 的 ReReload 段，功能窗口在蒙太奇内（RH Reverse Just Reload Window）。 */
	UFUNCTION(BlueprintCallable, Category = "RH|Action")
	void HandleReReload();

	/** 防御性招式统一输入：防御键按下/松开（蓝图各绑一次）。按武器 DefensiveType 分发（Parry/ReverseReload 只认按下，Defend 按住持续）。 */
	UFUNCTION(BlueprintCallable, Category = "RH|Action")
	void HandleDefensiveInput(bool bPressed);

	/** 弹反成功（完美窗口接中敌人命中）：玩家进入自由态（当前动作全部可取消），并标记下次弹反换手。 */
	UFUNCTION(BlueprintCallable, Category = "RH|Action")
	void NotifyParrySuccess();

	/** 相杀窗口开关（RH Clash Window ANS 调用）。 */
	UFUNCTION(BlueprintCallable, Category = "RH|Action")
	void SetClashWindow(bool bOpen);

	/** 相杀结算：本方命中框与其它武器命中框相撞（由 hitbox 重叠调用）。 */
	UFUNCTION(BlueprintCallable, Category = "RH|Action")
	void HandleWeaponClash(AActor* OtherWeapon, const FVector& HitLocation);

	/** 闪击窗口开关（RH Blitz Window ANS 调用）：开启期间骨骼网格体接触的角色视同被武器命中框打中。 */
	UFUNCTION(BlueprintCallable, Category = "RH|Action")
	void SetBlitzWindow(bool bOpen);

	/**
	 * 闪击射线（RH Blitz AN 调用）：从玩家向锁定目标打射线（无锁定 = 角色 forward × 1000），
	 * 命中敌人直接按当前招式 DA（CurrentActionState.Resolved）结算伤害；HitboxTag 用于选择受击方向。
	 * OnomValue：None=跟随武器 AttackHitRule；否则本次命中按指定极性获得 Onom。
	 */
	UFUNCTION(BlueprintCallable, Category = "RH|Action")
	void ExecuteBlitz(const FGameplayTag& HitboxTag, ERHOnomValue OnomValue = ERHOnomValue::None);

	UFUNCTION(BlueprintPure, Category = "RH|Action")
	bool IsClashWindowOpen() const { return bClashWindowOpen; }

	UFUNCTION()
	void HandleBlitzOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/** 进入受击动画状态：挂 Busy、清取消窗口（受击蒙太奇播放期间调用）。 */
	UFUNCTION(BlueprintCallable, Category = "RH|Action")
	void EnterHitReactionState();

	/** 退出受击动画状态（受击蒙太奇结束/被打断时调用，回到 Idle）。 */
	UFUNCTION(BlueprintCallable, Category = "RH|Action")
	void ExitHitReactionState();

	/** 玩家闪避成功躲掉一次命中：全局时间膨胀短暂变缓再恢复（敌人不触发）。 */
	UFUNCTION(BlueprintCallable, Category = "RH|Action")
	void TriggerDodgeTimeDilation(float TimeScale = 0.f, float Duration = 0.f);

	/** 闪避成功追加反馈（音效 + mesh 覆盖材质 + 相机 rig）；共鸣时间奖励走基类。 */
	virtual void HandleDodgeSuccess() override;

	/** 触发 just load：基类执行 load 效果+音效；bPlayVFX 时补播 parry 同款 VFX（Neutral）。 */
	virtual void ExecuteJustLoad(bool bPlayVFX) override;

	/** 当前普攻 Move 的命中 Onom 规则覆盖（bOverrideHitOnom 时返回）。 */
	virtual bool GetHitOnomRuleOverride(FRHOnomSourceRule& OutRule) const override;

	/** 触发弹反成功效果：开取消窗口 + 防御震屏。 */
	virtual void TriggerParryEffect() override;

	/** Just Load 窗口开关（RH Just Load Window ANS 调用）：窗口内被击中 → load + 命中无效化。 */
	UFUNCTION(BlueprintCallable, Category = "RH|Action")
	void OpenJustLoadWindow();

	UFUNCTION(BlueprintCallable, Category = "RH|Action")
	void CloseJustLoadWindow();

	UFUNCTION(BlueprintPure, Category = "RH|Action")
	bool IsJustLoadWindowActive() const { return bJustLoadWindowActive; }

	/** Just Reload 窗口（RH Just Reload Window ANS）：被击中 → 逆装填（消耗 ConsumeSeconds 秒共鸣）+ 命中无效化。 */
	UFUNCTION(BlueprintCallable, Category = "RH|Action")
	void OpenJustReloadWindow(float ConsumeSeconds);

	UFUNCTION(BlueprintCallable, Category = "RH|Action")
	void CloseJustReloadWindow();

	/** Reverse Just Reload 窗口（RH Reverse Just Reload Window ANS）：打开时立即消耗 ConsumeSeconds 秒共鸣（沉没成本，无共鸣则窗口不生效）；窗口内被击中 → 按预消耗记录的共鸣类型反色生成音形 + 命中无效化。 */
	UFUNCTION(BlueprintCallable, Category = "RH|Action")
	void OpenReverseJustReloadWindow(float ConsumeSeconds);

	UFUNCTION(BlueprintCallable, Category = "RH|Action")
	void CloseReverseJustReloadWindow();

	/** 任一 Just 窗口命中分发：前置条件满足返回 true（命中已处理=无效化）；不满足返回 false（命中照常结算，窗口空转）。 */
	UFUNCTION(BlueprintCallable, Category = "RH|Action")
	bool TryHandleJustWindowHit(const FRHHitData& HitData);

	/** 闪避成功反馈资产组（音效/覆盖材质/相机 rig；时长共用 DodgeTimeDilationDuration）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RH|Dodge")
	FRHDodgeFeedback DodgeFeedback;

	/** 闪避成功慢动作：时间膨胀系数（越小越慢；0 时用此值）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RH|Dodge", meta = (ClampMin = "0.01", ClampMax = "1"))
	float DodgeTimeDilationScale = 0.25f;

	/** 闪避成功慢动作：持续时间（秒；0 时用此值）。材质/相机 rig 也持续同样时长。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RH|Dodge", meta = (ClampMin = "0"))
	float DodgeTimeDilationDuration = 0.2f;

	/**
	 * 装填/抛弹统一入口：手牌含灰色音形 → 抛弹动画（清手牌 + 完美防御窗口），
	 * 无灰色且存在非灰音形 → 装填动画（存入共鸣）。
	 * load 本身作为 Special 类型取消：当前动作开了 Window.Cancel.Special 时可直接打断。
	 * 真正的执行点仍由动画上的 RH Load / RH Toss 通知触发（动画不同，两条通知都保留）。
	 * 是否速装/速抛由 Window.AllowFast 决定。
	 */
	UFUNCTION(BlueprintCallable, Category = "RH|Action")
	void HandleLoad();

	/** 装填/抛弹统一执行点：AN_Load 通知调用（有灰色→清手牌抛弹，无灰色→存入共鸣）。 */
	UFUNCTION(BlueprintCallable, Category = "RH|Action")
	void ExecuteLoad();

	/** 逆向装填执行点（RH Reload AN 调用）：共鸣按等级/类型反向生成手牌音形，消耗 ConsumeSeconds 秒共鸣时长。 */
	UFUNCTION(BlueprintCallable, Category = "RH|Action")
	void ExecuteReload(float ConsumeSeconds);

	/** 兼容入口：行为与 HandleLoad 完全一致（直接转发），旧的 IA_Toss 绑定无需改。 */
	UFUNCTION(BlueprintCallable, Category = "RH|Action")
	void HandleToss();

	/** 终结技：轰鸣蓄满后释放（无消耗动作），成功返回 true。 */
	UFUNCTION(BlueprintCallable, Category = "RH|Action")
	bool TryReleaseFinisher();

	/** 切换技：播放当前武器 DA 的 SwitchTactic（无消耗），成功返回 true。 */
	UFUNCTION(BlueprintCallable, Category = "RH|Action")
	bool TryPlaySwitchTactic();

	/** 动作蒙太奇时间点触发效果（AN_CastEffect 调用）：从当前动作 DA 激活 EffectAbility。 */
	UFUNCTION(BlueprintCallable, Category = "RH|Action")
	void HandleCastEffect();

	UFUNCTION(BlueprintPure, Category = "RH|Action")
	bool IsIdle() const;

	UFUNCTION(BlueprintPure, Category = "RH|Action")
	ERHActionState GetActionState() const;

	UFUNCTION(BlueprintCallable, Category = "RH|Action")
	void SetComboWindowOpen(bool bOpen);

	/** 派生战技链窗口：RH Chain Window ANS 驱动，开启期间战技键可派生到 NextActions 下一段。 */
	UFUNCTION(BlueprintCallable, Category = "RH|Action")
	void SetChainWindowOpen(bool bOpen);

	/**
	 * 闪避派生窗口（Action.Move.Dodge tag）：翻滚中 / 翻滚与防御正常结束 / 防御主动退出时挂上，
	 * 段数为 0 的下一次普攻或翻滚取消派生 DodgeAttackMoveDefinition——与跑步派生（Action.Move.Run）同一套 tag 驱动。
	 */
	UFUNCTION(BlueprintCallable, Category = "RH|Action")
	void SetDodgeDeriveTag(bool bEnabled);

	/** 当前派生链深度（第几段，0 = 不在链上/链头起手）。 */
	UFUNCTION(BlueprintPure, Category = "RH|Action")
	int32 GetChainDepth() const { return CurrentChainDepth; }

	UFUNCTION(BlueprintCallable, Category = "RH|Action")
	void SetPreInputWindowOpen(bool bOpen);

	UFUNCTION(BlueprintPure, Category = "RH|Action")
	bool HasPendingAction() const;

	UFUNCTION(BlueprintCallable, Category = "RH|Action")
	void CancelAction();

	UFUNCTION(BlueprintCallable, Category = "RH|Combat")
	void InitializeAfterAbilitySystem();

	/** 最近一次动作施放上下文（效果 GA 读取：来源/动作 DA/消耗/折扣/伤害）。 */
	UFUNCTION(BlueprintPure, Category = "RH|Action")
	FRHOnomActionContext GetLastActionContext() const;

	/** 对应取消 tag 是否开启（该行为当前可插入）。 */
	UFUNCTION(BlueprintPure, Category = "RH|Cancel")
	bool HasCancelTag(ERHCancelType Type) const;

	/** 打开某个取消 tag（RH Cancel Notify 调用，动作开始/结束统一清除）。 */
	UFUNCTION(BlueprintCallable, Category = "RH|Cancel")
	void OpenCancel(ERHCancelType Type);

	/** 清除全部取消 tag（动作开始/结束时调用）。 */
	UFUNCTION(BlueprintCallable, Category = "RH|Cancel")
	void ClearCancelTags();

	/** 打断当前动作时清理未结算的动作上下文/消耗/武器覆层/施放极性（普攻取消战技等）。 */
	void ClearActionCastState();

	/** Temporary weapon swap: hides current weapon, spawns the DA's weapon class on the DA's socket, restores when the action ends. */
	UFUNCTION(BlueprintCallable, Category = "RH|Weapon")
	void EnterTemporaryWeapon(URHWeaponDefinition* InWeaponDefinition);

	/** 退出防御态：清 bBlocking 并关 Status.Guarding（被打断/破防/退出 block 时调用）。 */
	void ClearBlockState();
	void PlayClashFeedback(const FVector& HitLocation);

protected:
	UFUNCTION()
	void HandleActionMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	void StartAttackChain();
	void AdvanceAttack();
	/** 快速装填后续连段：当前段数 +1 继续，成功返回 true（消费 bPreserveCombo）。 */
	bool TryContinueComboAfterLoad();

	/** 开连段桥：置 bPreserveCombo 并按武器 ComboBridgeTimeout 起超时（0=不超时）。 */
	void StartComboBridge();

	/**
	 * 尝试派生闪避攻击：持有 Action.Move.Dodge tag 且段数为 0 时播 DodgeAttackMoveDefinition 并消费窗口。
	 * HandleAttack 的 Idle 分支与翻滚取消分支共用，与跑步派生（查 Action.Move.Run）同构。
	 */
	bool TryPlayDodgeAttack();

	/** 清连段桥：置 false 并取消超时（消费桥/其它动作起手/取消动作时调用）。 */
	void ClearComboBridge();

	/** 连段桥超时：段数归零、桥作废。 */
	void HandleComboBridgeTimeout();
	void PlayMove(URHMoveDefinition* Move, ERHActionState NewState);
	bool TryStartActionFlow(URHOnomActionDefinition* Action);
	void StartAction(URHOnomActionDefinition* Action, const FRHOnomResolvedAction& Resolved);
	void PlayEffectAbility(TSubclassOf<UGameplayAbility> AbilityClass);
	void RestoreTemporaryWeapon();
	bool CanConsumeOnom(int32 Amount) const;
	bool TryConsumeOnom(int32 Amount);
	void StorePendingAttack();
	void StorePendingSkill(int32 SkillIndex);
	void StorePendingRhythmWeapon(URHOnomActionDefinition* Action);
	void ConsumePendingAction();
	void SetActionState(ERHActionState NewState);
	void ApplyHealthInitializer();
	void PlayLoadMontage();
	void PlayTossMontage();
	bool IsFastAllowed() const;
	FName ResolveDodgeSection() const;

	UPROPERTY(Transient)
	ERHActionState ActionState = ERHActionState::Idle;

	UPROPERTY(Transient)
	int32 CurrentAttackIndex = 0;

	UPROPERTY(Transient)
	int32 CastEffectIndex = 0;

	UPROPERTY(Transient)
	bool bTemporaryWeaponActive = false;

	UPROPERTY(Transient)
	TObjectPtr<AWeaponBase> TemporaryWeaponActor;

	UPROPERTY(Transient)
	TObjectPtr<AWeaponBase> OriginalTemporaryWeapon;

	UPROPERTY(Transient)
	TObjectPtr<URHMoveDefinition> CurrentMoveDefinition;

	UPROPERTY(Transient)
	FGameplayTagContainer ActiveMoveGrantedTags;

	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> CurrentActionMontage;

	UPROPERTY(Transient)
	bool bComboWindowOpen = false;

	/** 派生战技链窗口（RH Chain Window ANS 开/关）。 */
	UPROPERTY(Transient)
	bool bChainWindowOpen = false;

	/** 当前派生链深度：链段派生时 +1，动作结束/取消归 0（供动画蓝图/UI 显示段数）。 */
	UPROPERTY(Transient)
	int32 CurrentChainDepth = 0;

	UPROPERTY(Transient)
	bool bPreInputWindowOpen = false;

	UPROPERTY(Transient)
	bool bHasPendingAction = false;

	UPROPERTY(Transient)
	ERHActionState PendingActionState = ERHActionState::Idle;

	UPROPERTY(Transient)
	int32 PendingAttackIndex = 0;

	UPROPERTY(Transient)
	int32 PendingSkillIndex = INDEX_NONE;

	UPROPERTY(Transient)
	TObjectPtr<URHOnomActionDefinition> PendingRhythmWeapon;

	UPROPERTY(Transient)
	FRHOnomActionContext LastActionContext;

	UPROPERTY(Transient)
	FTimerHandle DodgeTimeDilationTimerHandle;

	/** 连段桥超时计时器（快速装填/闪避保留段数后到点作废）。 */
	UPROPERTY(Transient)
	FTimerHandle ComboBridgeTimerHandle;

	UPROPERTY(Transient)
	bool bBlocking = false;

	/** 快速装填（Window.AllowFast）保留普攻连段计数；普通装填/其它动作会清掉。 */
	UPROPERTY(Transient)
	bool bPreserveCombo = false;

	/** 弹反成功后翻转：下次弹反用 R（L 起手，成功后 L↔R 轮换）。 */
	UPROPERTY(Transient)
	bool bParryNextSectionIsR = false;

	UPROPERTY(Transient)
	bool bClashWindowOpen = false;

	UPROPERTY(Transient)
	bool bBlitzWindowOpen = false;

	UPROPERTY(Transient)
	bool bJustLoadWindowActive = false;

	UPROPERTY(Transient)
	bool bJustReloadWindowActive = false;

	UPROPERTY(Transient)
	float JustReloadConsumeSeconds = 20.f;

	UPROPERTY(Transient)
	bool bReverseJustReloadWindowActive = false;

	/** parry 同款反馈（固定 ParrySound + ParryFeedback Neutral 档 VFX），Just Load 用。 */
	void PlayJustLoadFeedback(const FVector& HitLocation);
};
