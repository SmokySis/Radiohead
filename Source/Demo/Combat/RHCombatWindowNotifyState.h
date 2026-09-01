#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "RHCombatWindowNotifyState.generated.h"

/**
 * RH Combat Window（RH 战斗/连段窗口）
 * 所属系统：RH（RadioHead Onom）
 * 驱动组件：URHCombatComponent（经 IRHCombatActionInterface::SetComboWindowOpen）
 * 用途：窗口内允许接下一段攻击/动作（连段输入缓冲）
 * 放置：普攻/动作蒙太奇的连段窗口段
 * 参数：无
 */
UCLASS(meta = (DisplayName = "RH Combat Window"))
class DEMO_API URHCombatWindowNotifyState : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};
