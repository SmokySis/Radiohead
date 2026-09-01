#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "RHJustLoadWindowNotifyState.generated.h"

/**
 * RH Just Load Window（精准装填窗口）：
 * 窗口开放期间被攻击 → 立即触发 just load（装填/抛弹效果+音效+parry 同款反馈），
 * 并且让本次命中无效化（不扣血/不结算/不触发受击）。
 */
UCLASS(meta = (DisplayName = "RH Just Load Window"))
class DEMO_API URHJustLoadWindowNotifyState : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};
