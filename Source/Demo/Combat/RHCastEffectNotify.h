#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "RHCastEffectNotify.generated.h"

/**
 * RH Cast Effect（施放效果触发点，时间点 Notify）：
 * 动作蒙太奇的指定帧通知战斗组件触发效果（GA 类与系数全部从当前动作 DA 读取）。
 */
UCLASS(meta = (DisplayName = "RH Cast Effect"))
class DEMO_API URHCastEffectNotify : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};
