#pragma once

#include "CoreMinimal.h"
#include "RHOnomTypes.generated.h"

class AActor;
class URHOnomActionDefinition;

UENUM(BlueprintType)
enum class ERHOnomValue : uint8
{
	None,
	Positive,
	Negative,
	Broken,
	/** 平调音形：和值贡献 0，可用（参与装填/消耗），非灰色。 */
	Neutral
};

/** 共鸣极性：由装填音形的和值决定（和>0 大调，和<0 小调，和=0 平调）。 */
UENUM(BlueprintType)
enum class ERHOnomPolarity : uint8
{
	None,
	Major,
	Minor,
	Neutral,
	/** 破碎音形（灰色）。 */
	Broken
};

UENUM(BlueprintType)
enum class ERHOnomRuleMode : uint8
{
	Add,
	Remove
};

/** 技能绝对值和值判定模式。 */
UENUM(BlueprintType)
enum class ERHOnomRequirementMode : uint8
{
	/** 限定绝对值：|和值| 不足时无法释放。 */
	Limited,
	/** 不限绝对值：任意和值均可释放，效果不打折。 */
	Unrestricted,
	/** 不限但打折：可以释放，但 |和值| 不足时效果按比例打折（默认 x0.7）。 */
	UnrestrictedWithDiscount
};

/** 灰色破碎音形进入槽位的判定结果（普通防御被击中路径）。 */
UENUM(BlueprintType)
enum class ERHOnomGuardOutcome : uint8
{
	None,
	/** 正常放入空槽。 */
	Added,
	/** 槽满：灰色吞掉 1 个现有 Onom 后放入。 */
	Swallowed,
	/** 灰色达到阈值，触发大破防：清空灰色、失去共鸣、不扣轰鸣。 */
	BigBreak
};

/** 受伤时的音形结算模式（由音律核心决定）。 */
UENUM(BlueprintType)
enum class ERHOnomDamageTakenMode : uint8
{
	/** 默认：清空全部 Onom 槽，共鸣 -2s。 */
	ClearAll,
	/** 按音律核心的规则结算（失去/增加/自定义数量）。 */
	ApplyRule
};

/** 闪避成功/受击等场景的可配置效果类型（由效果数组驱动，替代写死的单一结算）。 */
UENUM(BlueprintType)
enum class ERHOnomEffectType : uint8
{
	None,
	/** 共鸣时长增减：Amount 秒（正=增加，负=减少）。 */
	AddResonanceTime,
	/** 音形增减：Count 个（正=获得，负=减少），ValueType 指定获得时的音形类型。 */
	ModifyOnomCount,
	/** 触发 just load（无蒙太奇直接装填/抛弹）：bPlayVFX 控制是否播 parry 同款 VFX。 */
	TriggerJustLoad,
	/** 触发弹反成功效果（开取消窗口等）。 */
	TriggerParry
};

/** 单个效果条目：字段按 Type 各取所需。 */
USTRUCT(BlueprintType)
struct FRHOnomEffect
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
	ERHOnomEffectType Type = ERHOnomEffectType::None;

	/** AddResonanceTime：秒数（负=减少）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect", meta = (EditCondition = "Type == ERHOnomEffectType::AddResonanceTime", EditConditionHides))
	float Amount = 0.f;

	/** ModifyOnomCount：获得时指定音形类型（Positive/Negative/Broken）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect", meta = (EditCondition = "Type == ERHOnomEffectType::ModifyOnomCount", EditConditionHides))
	ERHOnomValue ValueType = ERHOnomValue::Positive;

	/** ModifyOnomCount：数量（正=获得，负=减少任意手牌）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect", meta = (EditCondition = "Type == ERHOnomEffectType::ModifyOnomCount", EditConditionHides))
	int32 Count = 0;

	/** TriggerJustLoad：是否播放 parry 同款 VFX（polarity=Neutral）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect", meta = (EditCondition = "Type == ERHOnomEffectType::TriggerJustLoad", EditConditionHides))
	bool bPlayVFX = true;
};

USTRUCT(BlueprintType)
struct FRHOnomSourceRule
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Onom")
	bool bEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Onom")
	ERHOnomRuleMode Mode = ERHOnomRuleMode::Add;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Onom")
	ERHOnomValue Type = ERHOnomValue::Positive;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Onom", meta = (ClampMin = "0"))
	int32 Count = 1;
};

USTRUCT(BlueprintType)
struct FRHOnomState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Onom")
	TArray<ERHOnomValue> Slots;

	UPROPERTY(BlueprintReadOnly, Category = "Onom")
	int32 ResonanceLayers = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Onom")
	ERHOnomPolarity ResonanceType = ERHOnomPolarity::None;

	UPROPERTY(BlueprintReadOnly, Category = "Onom")
	int32 ResonanceSignedSum = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Onom")
	float ResonanceTimeRemaining = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Onom")
	float ChargePercent = 0.f;
};

USTRUCT(BlueprintType)
struct FRHOnomEventData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Onom")
	ERHOnomValue Type = ERHOnomValue::None;

	UPROPERTY(BlueprintReadOnly, Category = "Onom")
	int32 Count = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Onom")
	AActor* Instigator = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Onom")
	FRHOnomState State;
};

USTRUCT(BlueprintType)
struct FRHOnomResonanceEventData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Onom")
	int32 OldLayers = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Onom")
	int32 NewLayers = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Onom")
	ERHOnomPolarity Type = ERHOnomPolarity::None;

	UPROPERTY(BlueprintReadOnly, Category = "Onom")
	int32 SignedSum = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Onom")
	float TimeRemaining = 0.f;
};

/** 一次精确消耗的结果。ConsumedCount = HandConsumed + 共鸣 1 格。 */
USTRUCT(BlueprintType)
struct FRHOnomConsumptionData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Onom")
	int32 ConsumedCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Onom")
	int32 HandConsumed = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Onom")
	bool bResonanceConsumed = false;

	/** 共鸣被消耗时的等级（未消耗为 0）。 */
	UPROPERTY(BlueprintReadOnly, Category = "Onom")
	int32 ResonanceLevel = 0;

	/** 共鸣被消耗时的极性（未消耗为 None）。 */
	UPROPERTY(BlueprintReadOnly, Category = "Onom")
	ERHOnomPolarity ResonanceType = ERHOnomPolarity::None;

	UPROPERTY(BlueprintReadOnly, Category = "Onom")
	int32 SignedSum = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Onom")
	int32 AbsoluteSum = 0;
};

/** 由消耗数据解析极性（和>0 大调、和<0 小调、和=0 且消耗过为平调）。 */
inline ERHOnomPolarity RHGetConsumptionPolarity(const FRHOnomConsumptionData& Data)
{
	if (Data.SignedSum > 0)
	{
		return ERHOnomPolarity::Major;
	}
	if (Data.SignedSum < 0)
	{
		return ERHOnomPolarity::Minor;
	}
	if (Data.ConsumedCount > 0)
	{
		return ERHOnomPolarity::Neutral;
	}
	return ERHOnomPolarity::None;
}

/** 战技/魔法共用的消耗要求判定：数目不足一律拒绝；限定模式再校验极性（None=不限极性）。 */
inline bool RHOnomMatchesRequirement(int32 RequiredCount, ERHOnomPolarity RequiredPolarity, ERHOnomRequirementMode Mode, const FRHOnomConsumptionData& Data)
{
	if (Data.ConsumedCount < RequiredCount)
	{
		return false;
	}

	switch (Mode)
	{
	case ERHOnomRequirementMode::Limited:
		return RequiredPolarity == ERHOnomPolarity::None || RHGetConsumptionPolarity(Data) == RequiredPolarity;
	case ERHOnomRequirementMode::Unrestricted:
	case ERHOnomRequirementMode::UnrestrictedWithDiscount:
	default:
		return true;
	}
}

/** 共鸣被消耗时的增幅结果（伤害 / 共振伤害 / 轰鸣获取倍率）。 */
USTRUCT(BlueprintType)
struct FRHOnomAmplification
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Onom")
	float DamageMultiplier = 1.f;

	UPROPERTY(BlueprintReadOnly, Category = "Onom")
	float ResonanceDamageMultiplier = 1.f;

	UPROPERTY(BlueprintReadOnly, Category = "Onom")
	float ChargeMultiplier = 1.f;
};

/** 灰色大破防事件数据。 */
USTRUCT(BlueprintType)
struct FRHOnomBigBreakData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Onom")
	float FixedDamage = 15.f;

	UPROPERTY(BlueprintReadOnly, Category = "Onom")
	float StunSeconds = 2.f;
};

/** 动作施放上下文：效果 GA 在触发时读取（来源、动作 DA、消耗、折扣、伤害）。 */
USTRUCT(BlueprintType)
struct FRHOnomActionContext
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Onom")
	AActor* Source = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Onom")
	TObjectPtr<URHOnomActionDefinition> ActionDefinition;

	UPROPERTY(BlueprintReadOnly, Category = "Onom")
	FRHOnomConsumptionData Consumption;

	/** 本次施放的极性（由消耗和值决定，无消耗动作 = None）。 */
	UPROPERTY(BlueprintReadOnly, Category = "Onom")
	ERHOnomPolarity Polarity = ERHOnomPolarity::None;

	UPROPERTY(BlueprintReadOnly, Category = "Onom")
	float DiscountMultiplier = 1.f;

	UPROPERTY(BlueprintReadOnly, Category = "Onom")
	int32 Damage = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Onom")
	int32 ResonanceDamage = 0;
};

USTRUCT(BlueprintType)
struct FRHOnomReactionOverride
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Onom|Override")
	bool bSuppressOnomGain = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Onom|Override")
	bool bSuppressChargeGain = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Onom|Override")
	bool bOverrideOnomHitRule = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Onom|Override")
	FRHOnomSourceRule OverrideRule;
};
