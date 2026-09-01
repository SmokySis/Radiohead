#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Demo/Onom/RHOnomTypes.h"
#include "RHOnomActionDefinition.generated.h"

class UAnimMontage;

/** 按和值极性区分的招式变体（例如双刀：2 大调=阳炎斩、2 小调=霜月斩、平调=回旋斩）。 */
USTRUCT(BlueprintType)
struct FRHOnomActionVariant
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Variant")
	ERHOnomPolarity Polarity = ERHOnomPolarity::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Variant")
	TSoftObjectPtr<UAnimMontage> Montage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Variant")
	FName SectionName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Variant", meta = (ClampMin = "0.01"))
	float PlayRate = 1.f;

	/** >0 时覆盖动作基础伤害。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Variant", meta = (ClampMin = "0"))
	int32 Damage = 0;

	/** >0 时覆盖动作基础共振伤害。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Variant", meta = (ClampMin = "0"))
	int32 ResonanceDamage = 0;

	/** 勾选后使用下方 CounterBarDamage 覆盖动作基础反击条伤害（0 = 不覆盖，仍用基础值）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Variant")
	bool bOverrideCounterBarDamage = false;

	/** 勾选 bOverrideCounterBarDamage 后生效：对敌人反击条的扣减值（负数 = 给敌人回反击条）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Variant", meta = (EditCondition = "bOverrideCounterBarDamage", ToolTip = "对敌人反击条的扣减值（负数 = 给敌人回反击条）"))
	float CounterBarDamage = 0.f;

	/** >0 时覆盖动作基础 HitPoise。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Variant", meta = (ClampMin = "0"))
	int32 HitPoise = 0;
};

/** 动作按本次消耗解析出的实际执行数据（基础字段或极性变体）。 */
USTRUCT(BlueprintType)
struct FRHOnomResolvedAction
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Action")
	TSoftObjectPtr<UAnimMontage> Montage;

	UPROPERTY(BlueprintReadOnly, Category = "Action")
	FName SectionName;

	UPROPERTY(BlueprintReadOnly, Category = "Action")
	float PlayRate = 1.f;

	UPROPERTY(BlueprintReadOnly, Category = "Action")
	FGameplayTag AttackTag;

	UPROPERTY(BlueprintReadOnly, Category = "Action")
	FGameplayTagContainer GrantedTags;

	UPROPERTY(BlueprintReadOnly, Category = "Action")
	int32 HitStopLevel = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Action")
	int32 PoiseLevel = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Action")
	int32 HitPoise = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Action")
	float Resistance = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Action")
	int32 Damage = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Action")
	int32 ResonanceDamage = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Action")
	float CounterBarDamage = 0.f;
};

/**
 * 统一动作定义（战技 / 音律武器共用）：
 * 消耗手牌+共鸣（先共鸣 1 格、手牌补足），命中首次结算轰鸣；
 * 可选 EffectAbility 承载特殊效果（弹幕 / 增益 / 隐形 / 特殊闪避等）。
 */
UCLASS(BlueprintType)
class DEMO_API URHOnomActionDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AssetRegistrySearchable, Category = "Action")
	FName ActionId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action")
	FText DisplayName;

	/** 要求的消耗极性（None=不限；小调/蓝色技能选 Minor，大调选 Major，平调选 Neutral）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|Requirement")
	ERHOnomPolarity RequirementPolarity = ERHOnomPolarity::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|Requirement", meta = (ClampMin = "1"))
	int32 RequiredCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|Requirement")
	ERHOnomRequirementMode RequirementMode = ERHOnomRequirementMode::Unrestricted;

	/** 不限但打折时的折扣（默认 x0.7）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|Requirement", meta = (ClampMin = "0", ClampMax = "1"))
	float DiscountFactor = 0.7f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action", meta = (ClampMin = "0"))
	int32 Priority = 0;

	/** 动作伤害；0 表示不直接造成面板伤害（纯增益/投射物由 EffectAbility 结算）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action", meta = (ClampMin = "0"))
	int32 Damage = 0;

	/** 动作共振伤害。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action", meta = (ClampMin = "0"))
	int32 ResonanceDamage = 0;

	/** 动作对敌人反击条的扣减值（负数 = 给敌人回反击条）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action", meta = (ToolTip = "动作对敌人反击条的扣减值（负数 = 给敌人回反击条）"))
	float CounterBarDamage = 0.f;

	// ---------------------------------------------------------------------
	// 招式表现（动作自包含，不引用 Move Definition）
	// ---------------------------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|Move")
	TSoftObjectPtr<UAnimMontage> Montage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|Move")
	FName SectionName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|Move", meta = (ClampMin = "0.01"))
	float PlayRate = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|Move")
	FGameplayTag AttackTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|Move")
	FGameplayTagContainer GrantedTags;

	/** AI intent tags (Melee/Ranged/Heal), mounted on ASC while playing so enemy AI can read. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|Move")
	FGameplayTagContainer AIIntentTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|Move", meta = (ClampMin = "0"))
	int32 HitStopLevel = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|Move", meta = (ClampMin = "0"))
	int32 PoiseLevel = 0;

	/** 执行该动作时被命中受到的减伤系数（0~1，独立于打断判定）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|Move", meta = (ClampMin = "0", ClampMax = "1"))
	float Resistance = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|Move")
	TArray<FRHOnomActionVariant> Variants;

	/** Attack poise damage level. Falls back to PoiseLevel when left at 0. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|Move", meta = (ClampMin = "0"))
	int32 HitPoise = 0;

	// ---------------------------------------------------------------------
	// 特殊效果（可选 GA：弹幕 / 增益 / 隐形 / 特殊闪避等）
	// ---------------------------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|Effect")
	TSubclassOf<UGameplayAbility> EffectAbility;

	/** Multiple GameplayAbilities: each AN_CastEffect activates the next one in order. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|Effect")
	TArray<TSubclassOf<UGameplayAbility>> EffectAbilities;

	// ---------------------------------------------------------------------
	// 派生战技（线性连招）：本动作播放期间链窗口（RH Chain Window）开启时，
	// 按下战技键按数组顺序检测，第一个消耗/蒙太奇都满足的动作立即进入，不再检测后面的。
	// 空数组 = 链终点。每段是独立 DA，各自带自己的消耗/伤害/变体/EffectAbility。
	// ---------------------------------------------------------------------

	/** 派生链下一段候选（按顺序检测，首个符合即进入；都不符合则本次按键不生效）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action|Chain")
	TArray<TObjectPtr<URHOnomActionDefinition>> NextActions;


	UFUNCTION(BlueprintCallable, Category = "Action")
	bool MatchesConsumption(const FRHOnomConsumptionData& Data) const;

	/** 按消耗和值极性解析实际执行数据（变体优先，否则基础字段）。返回 false 表示没有可用蒙太奇。 */
	UFUNCTION(BlueprintCallable, Category = "Action")
	bool ResolveActionData(const FRHOnomConsumptionData& Data, FRHOnomResolvedAction& OutData) const;

	/** 不限但打折时返回折扣系数，否则 1.0。 */
	UFUNCTION(BlueprintPure, Category = "Action")
	float GetDiscountMultiplier(const FRHOnomConsumptionData& Data) const;
};
