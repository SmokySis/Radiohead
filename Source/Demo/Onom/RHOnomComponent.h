#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Demo/Onom/RHOnomTypes.h"
#include "Engine/TimerHandle.h"
#include "RHOnomComponent.generated.h"

class URHCoreDefinition;
class URHDodgeCoreDefinition;
class URHOnomSettings;
class URHOnomGainFeedbackDefinition;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRHOnomStateChanged, const FRHOnomState&, State);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRHOnomEvent, const FRHOnomEventData&, Event);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRHOnomResonanceEvent, const FRHOnomResonanceEventData&, Event);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FRHOnomStorageLevelChanged, int32, OldLayers, int32, NewLayers);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRHOnomChargeChanged, float, Percent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FRHOnomChargeFull);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FRHOnomResonanceExpired);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRHOnomBigBreak, const FRHOnomBigBreakData&, Data);

UCLASS(ClassGroup = (RadioHead), meta = (BlueprintSpawnableComponent))
class DEMO_API URHOnomComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	URHOnomComponent();

	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Onom|Config")
	TObjectPtr<URHOnomSettings> Settings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Onom|Core")
	TObjectPtr<URHCoreDefinition> CoreDefinition;

	/** 闪避成功效果配置（Dodge Core DA）：闪避成功时按效果数组结算，数组为空时回落 Settings 的共鸣奖励。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Onom|Core")
	TObjectPtr<URHDodgeCoreDefinition> DodgeCoreDefinition;

	/** 获得 Onom 的音效配置（玩家独有；敌人没有 Onom 组件）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Onom|Feedback")
	TObjectPtr<URHOnomGainFeedbackDefinition> GainFeedbackDefinition;

	UPROPERTY(BlueprintAssignable, Category = "Onom|Events")
	FRHOnomStateChanged OnOnomStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "Onom|Events")
	FRHOnomEvent OnOnomAdded;

	UPROPERTY(BlueprintAssignable, Category = "Onom|Events")
	FRHOnomEvent OnOnomRemoved;

	UPROPERTY(BlueprintAssignable, Category = "Onom|Events")
	FRHOnomEvent OnOnomCleared;

	UPROPERTY(BlueprintAssignable, Category = "Onom|Events")
	FRHOnomResonanceEvent OnResonanceStored;

	UPROPERTY(BlueprintAssignable, Category = "Onom|Events")
	FRHOnomStorageLevelChanged OnStorageLevelChanged;

	UPROPERTY(BlueprintAssignable, Category = "Onom|Events")
	FRHOnomResonanceEvent OnResonanceTimeChanged;

	UPROPERTY(BlueprintAssignable, Category = "Onom|Events")
	FRHOnomResonanceExpired OnResonanceExpired;

	UPROPERTY(BlueprintAssignable, Category = "Onom|Events")
	FRHOnomChargeChanged OnChargeChanged;

	UPROPERTY(BlueprintAssignable, Category = "Onom|Events")
	FRHOnomChargeFull OnChargeFull;

	/** 灰色达到阈值触发大破防（资源侧已完成清灰/失去共鸣；固定伤害与硬直由战斗组件处理）。 */
	UPROPERTY(BlueprintAssignable, Category = "Onom|Events")
	FRHOnomBigBreak OnBigBreakTriggered;

	UFUNCTION(BlueprintCallable, Category = "Onom")
	int32 AddOnom(const FRHOnomSourceRule& Rule, AActor* Instigator = nullptr);

	/** 普通防御被击中：灰色进入槽位（槽满吞掉 1 个非灰 Onom）；灰到阈值后，下一次防御受击才触发大破防。 */
	UFUNCTION(BlueprintCallable, Category = "Onom")
	ERHOnomGuardOutcome AddBrokenOnom(AActor* Instigator = nullptr);

	UFUNCTION(BlueprintCallable, Category = "Onom")
	int32 RemoveOnom(int32 Count = 1);

	/** 将手牌中所有灰色音形转换为指定类型（相杀奖励等）：无灰色则不改动手牌，返回转换数量。目标为 None/Broken（灰→灰）时不处理。 */
	UFUNCTION(BlueprintCallable, Category = "Onom")
	int32 ConvertGreyOnom(ERHOnomValue Value);

	/** 装填共鸣：等级 = 当前非灰手牌数量（1~3），属性由和值决定；覆盖旧共鸣并重置倒计时。 */
	UFUNCTION(BlueprintCallable, Category = "Onom")
	bool TryStoreToResonance();

	/** 逆向装填（RH Reload）：按共鸣等级/类型生成手牌音形（大调→正、小调→负、平调→平），不覆盖非灰、可覆盖灰、再填空位；消耗 ConsumeSeconds 秒共鸣时长（默认 20 远大于持续时间=直接用完），放不下丢弃。返回实际放入数。bInvertColor=true 为逆转逆装填（红→蓝、蓝→红、平→平）。 */
	UFUNCTION(BlueprintCallable, Category = "Onom")
	int32 ReloadFromResonance(float ConsumeSeconds, bool bInvertColor = false);

	/** 逆转逆装填预消耗（RH Reverse Just Reload 窗口打开时调用）：记录共鸣类型与待生成数量（按消耗时长比例折算），立即消耗共鸣（沉没成本），实际生成延后到 DeliverReverseReload。无共鸣/参数无效返回 false（窗口不生效）。 */
	UFUNCTION(BlueprintCallable, Category = "Onom")
	bool PrepareReverseReload(float ConsumeSeconds);

	/** 逆转逆装填交付（RH Reverse Just Reload 命中时调用）：按 PrepareReverseReload 记录的共鸣类型反色生成手牌音形（红→蓝、蓝→红、平→平），返回实际放入数。 */
	UFUNCTION(BlueprintCallable, Category = "Onom")
	int32 DeliverReverseReload();

	/** 攻击命中攒共鸣：共鸣层 +1（clamp 上限），类型覆盖为 Type，刷新倒计时，衰减速率按 DecayRate 倍率（默认 1.0，调用方按当前武器 DA 配置传入）。 */
	UFUNCTION(BlueprintCallable, Category = "Onom")
	void AddResonanceLayer(ERHOnomPolarity Type, float DecayRate = 1.f);

	/** 设置共鸣衰减速率倍率（切武器等场景重置为 1.0，或按当前武器 DA 配置应用）。 */
	UFUNCTION(BlueprintCallable, Category = "Onom")
	void SetResonanceDecayRate(float InRate);

	/** 覆盖式设置共鸣（相杀奖励等）：直接设为指定类型与等级（clamp 到 [1, MaxLayers]），已有共鸣被覆盖，刷新倒计时，衰减回 1.0（同装填）。Type 为 None/Broken 时返回 false。 */
	UFUNCTION(BlueprintCallable, Category = "Onom")
	bool SetResonance(ERHOnomPolarity Type, int32 Level);

	/** 手牌音形值 → 共鸣极性映射（正→Major、负→Minor、平→Neutral、灰→Broken）。 */
	UFUNCTION(BlueprintPure, Category = "Onom")
	static ERHOnomPolarity GetPolarityFromValue(ERHOnomValue Value);

	/** 精确消耗：先扣共鸣（固定 1 格）、手牌补足，绝不超额；灰色随消耗一起清空。 */
	UFUNCTION(BlueprintCallable, Category = "Onom")
	FRHOnomConsumptionData ConsumeOnom(int32 Amount);

	/** 仅从手牌消耗（魔法用）：不扣共鸣，灰色一并清空。 */
	UFUNCTION(BlueprintCallable, Category = "Onom")
	bool TryConsumeHandOnom(int32 Amount, FRHOnomConsumptionData& OutData);

	/** 抛弹：清空全部手牌（含灰色），保留共鸣。 */
	UFUNCTION(BlueprintCallable, Category = "Onom")
	FRHOnomConsumptionData ClearHandOnom();

	/** 兼容保留：清空手牌与共鸣。 */
	UFUNCTION(BlueprintCallable, Category = "Onom")
	FRHOnomConsumptionData ConsumeAllOnom();

	/** 受伤：清空全部 Onom 槽，共鸣维持 -2 秒（不扣轰鸣）。 */
	UFUNCTION(BlueprintCallable, Category = "Onom")
	void ApplyDamageTakenRule(AActor* Instigator = nullptr);

	/** 共鸣被消耗时的增幅：类型 x 等级系数 x 武器喜好匹配系数。 */
	UFUNCTION(BlueprintPure, Category = "Onom")
	FRHOnomAmplification ComputeAmplification(const FRHOnomConsumptionData& Consumption, const class URHWeaponDefinition* Weapon = nullptr) const;

	UFUNCTION(BlueprintPure, Category = "Onom")
	bool HasGreyOnom() const;

	UFUNCTION(BlueprintPure, Category = "Onom")
	int32 GetNonGreyOnomCount() const;

	UFUNCTION(BlueprintPure, Category = "Onom")
	int32 GetOnomSignedSum() const;

	UFUNCTION(BlueprintPure, Category = "Onom")
	int32 GetResonanceLayers() const;

	UFUNCTION(BlueprintPure, Category = "Onom")
	ERHOnomPolarity GetResonanceType() const;

	UFUNCTION(BlueprintPure, Category = "Onom")
	int32 GetResonanceSignedSum() const;

	UFUNCTION(BlueprintPure, Category = "Onom")
	int32 GetTotalConsumableOnom() const;

	UFUNCTION(BlueprintPure, Category = "Onom")
	FRHOnomConsumptionData GetConsumptionData() const;

	/** 纯模拟消耗 Amount 个音形（与 ConsumeOnom 同顺序/同和值折算，不改任何状态）：释放前匹配/UI 可选判定用，保证预览极性=实际消耗极性。 */
	UFUNCTION(BlueprintPure, Category = "Onom")
	FRHOnomConsumptionData SimulateConsumeOnom(int32 Amount) const;

	/** 只统计手牌（不含共鸣）的消耗预览：魔法等“仅从手牌扣”的判定用。 */
	UFUNCTION(BlueprintPure, Category = "Onom")
	FRHOnomConsumptionData GetHandConsumptionData() const;

	UFUNCTION(BlueprintPure, Category = "Onom")
	bool CanConsumeOnom(int32 Amount) const;

	UFUNCTION(BlueprintCallable, Category = "Onom")
	bool TryConsumeOnom(int32 Amount);

	UFUNCTION(BlueprintPure, Category = "Onom")
	float GetChargePercent() const;

	UFUNCTION(BlueprintCallable, Category = "Onom")
	void AddCharge(float Percent);

	UFUNCTION(BlueprintCallable, Category = "Onom")
	void ResetCharge();

	UFUNCTION(BlueprintPure, Category = "Onom")
	float GetChargeMaxPercent() const;

	/** 指定共鸣等级的衰减时长（无 Settings 时回落到 10/8/6 默认值），UI 用。 */
	UFUNCTION(BlueprintPure, Category = "Onom")
	float GetResonanceDecaySecondsForLevel(int32 Level) const;

	/** 共鸣槽最大层数（3）。 */
	UFUNCTION(BlueprintPure, Category = "Onom")
	int32 GetMaxLayers() const;

	UFUNCTION(BlueprintCallable, Category = "Onom")
	void AddResonanceTime(float Seconds);

	UFUNCTION(BlueprintCallable, Category = "Onom")
	void SubtractResonanceTime(float Seconds);

	UFUNCTION(BlueprintPure, Category = "Onom")
	FRHOnomState GetOnomState() const;

	void PlayGainSound(ERHOnomValue Type);
	/** 装填成功：按装入共鸣的极性播放获得音效。 */
	void PlayResonanceStoredSound(ERHOnomPolarity InResonanceType);

protected:
	int32 GetSlotCount() const;
	float GetResonanceDecayForCurrentLayer() const;
	void ClearResonance(bool bBroadcastExpired);
	void StartResonanceTimer();
	void StopResonanceTimer();
	void HandleResonanceTimerElapsed();
	void BroadcastState() const;
	ERHOnomGuardOutcome AddBrokenOnomInternal(AActor* Instigator, bool bBroadcast);

	/** 放置规则共用：不覆盖非灰、可覆盖灰、再填空位；放不下丢弃。返回实际放入数。 */
	int32 PlaceOnomValue(ERHOnomValue Value, int32 Count);

	UPROPERTY(Transient)
	TArray<ERHOnomValue> Slots;

	UPROPERTY(Transient)
	int32 ResonanceLayers = 0;

	UPROPERTY(Transient)
	ERHOnomPolarity ResonanceType = ERHOnomPolarity::None;

	UPROPERTY(Transient)
	int32 ResonanceSignedSum = 0;

	UPROPERTY(Transient)
	float ResonanceTimeRemaining = 0.f;

	/** 共鸣衰减倍率：攻击攒共鸣 = 当前武器 DA 配置（默认 1.0，旧行为硬编码 2.0），装填/相杀/切武器 = 1.0。 */
	UPROPERTY(Transient)
	float ResonanceDecayRate = 1.f;

	UPROPERTY(Transient)
	float ChargePercent = 0.f;

	/** 逆转逆装填预消耗时记录的共鸣类型（DeliverReverseReload 反色生成用）。 */
	UPROPERTY(Transient)
	ERHOnomPolarity ReverseReloadType = ERHOnomPolarity::None;

	/** 逆转逆装填预消耗时记录的待生成数量（DeliverReverseReload 放置用）。 */
	UPROPERTY(Transient)
	int32 ReverseReloadPendingCount = 0;

	UPROPERTY(Transient)
	FTimerHandle ResonanceTimerHandle;
};
