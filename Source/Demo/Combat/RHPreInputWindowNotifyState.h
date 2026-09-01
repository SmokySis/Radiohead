#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "RHPreInputWindowNotifyState.generated.h"

/**
 * RH Pre-Input Window（RH 预输入窗口）
 * 所属系统：RH（RadioHead Onom）
 * 驱动组件：URHCombatComponent（经 IRHCombatActionInterface::SetPreInputWindowOpen）
 * 用途：窗口内按下动作会先缓存，等窗口结束/下一动作开始时消费（提前输入）
 * 放置：动作末尾的预输入段
 * 参数：无
 */
UCLASS(meta = (DisplayName = "RH Pre-Input Window"))
class DEMO_API URHPreInputWindowNotifyState : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};
