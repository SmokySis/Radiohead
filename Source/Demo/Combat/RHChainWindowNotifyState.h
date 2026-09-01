#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "RHChainWindowNotifyState.generated.h"

/**
 * RH Chain Window（派生战技窗口）
 * 所属系统：RH（RadioHead Onom）
 * 驱动组件：URHCombatComponent（FindComponentByClass）
 * 用途：窗口开启期间按下战技键，当前动作 DA 的 NextActions 数组按顺序检测，
 *       第一个消耗/蒙太奇满足的动作立即进入（线性派生连招，模式同普攻连段窗口）。
 * 放置：派生链段蒙太奇的“可接下一段”区间
 * 参数：无
 */
UCLASS(meta = (DisplayName = "RH Chain Window"))
class DEMO_API URHChainWindowNotifyState : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};
