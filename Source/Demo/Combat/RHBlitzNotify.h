#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "Demo/Onom/RHOnomTypes.h"
#include "GameplayTagContainer.h"
#include "RHBlitzNotify.generated.h"

/**
 * RH Blitz（闪击射线）：单点 AnimNotify。
 * 触发时从玩家向锁定目标打射线（无锁定 = 角色 forward × 1000），
 * 命中敌人直接按当前招式 DA 结算伤害。
 * HitboxTag 保留用于选择受击方向（HitReaction.Direction.{F,L,R,B}）。
 */
UCLASS(meta = (DisplayName = "RH Blitz"))
class DEMO_API URHBlitzNotify : public UAnimNotify
{
	GENERATED_BODY()

public:
	/** 用于选择受击方向的 HitboxTag（HitReaction.Direction.{F,L,R,B}，兼容 Front/Back/Left/Right）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blitz")
	FGameplayTag HitboxTag;

	/** 本次命中攻击方获得的 Onom 极性：None=跟随武器 AttackHitRule；否则按指定音形类型获得。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blitz")
	ERHOnomValue OnomPolarity = ERHOnomValue::None;

	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};
