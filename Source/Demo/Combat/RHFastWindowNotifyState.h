#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "RHFastWindowNotifyState.generated.h"

/**
 * RH Fast Window（快速装填/抛弹窗口）：
 * 窗口开放期间挂 Window.AllowFast tag，HandleLoad/HandleToss 据此选择速装/速抛动画。
 */
UCLASS(meta = (DisplayName = "RH Fast Window"))
class DEMO_API URHFastWindowNotifyState : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};
