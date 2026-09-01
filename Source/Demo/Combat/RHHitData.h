#pragma once

#include "CoreMinimal.h"
#include "Demo/Combat/KnsHitReactionTypes.h"
#include "Demo/Onom/RHOnomTypes.h"
#include "GameplayTagContainer.h"
#include "RHHitData.generated.h"

class AActor;

USTRUCT(BlueprintType)
struct FRHHitData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit")
	float Damage = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit")
	float ResonanceDamage = 0.f;

	/** 本次命中对敌人反击条的扣减值（由玩家招式定义提供）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit", meta = (ClampMin = "0"))
	float CounterBarDamage = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit", meta = (ClampMin = "0"))
	float GuardResonanceMultiplier = 0.7f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit")
	bool bIsResonanceSkill = false;

	/** 是否战技命中（用于轰鸣蓄能与技能反馈）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit")
	bool bIsSkill = false;

	/** 是否结算受击方 Onom 惩罚（投射物等非 Onom 命中可置 false）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit")
	bool bApplyOnomPenalty = true;

	/** 战技命中的轰鸣格数（手牌消耗 + 共鸣等级），首次命中时结算给攻击者。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit", meta = (ClampMin = "0"))
	int32 ChargeGaugeCount = 0;

	/** 轰鸣获取倍率（共鸣类型/等级/武器系数）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit", meta = (ClampMin = "0"))
	float ChargeMultiplier = 1.f;

	/** 攻击韧性等级（打断判定用，int32）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit", meta = (ClampMin = "0"))
	int32 PoiseLevel = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit")
	AActor* Source = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit")
	FVector HitLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit")
	float Knockback = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit")
	FGameplayTagContainer Tags;

	/** 攻击方 hitbox 手动标记的受击方向（HitReaction.Direction.{F,L,R,B}）；false 时按命中点计算方向。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit")
	bool bUseManualHitDirection = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit")
	EKnsHitDirection HitDirection = EKnsHitDirection::Front;

	/** 覆盖本次命中攻击方获得的 Onom 规则（如 Blitz 指定极性）；false 时按武器 AttackHitRule。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit")
	bool bOverrideOnomRule = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit")
	FRHOnomSourceRule OverrideOnomRule;
};
