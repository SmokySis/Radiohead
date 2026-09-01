#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "RHInvincibleWindowNotifyState.generated.h"

/**
 * RH Invincible Window（无敌帧窗口）：
 * 开启期间挂 Status.Invincible（命中完全跳过），关闭时摘下。
 * 放在闪避/翻滚等蒙太奇的对应帧段。
 */
UCLASS(meta = (DisplayName = "RH Invincible Window"))
class DEMO_API URHInvincibleWindowNotifyState : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};
