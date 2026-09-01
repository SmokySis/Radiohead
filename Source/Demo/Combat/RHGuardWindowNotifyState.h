#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "RHGuardWindowNotifyState.generated.h"

/**
 * 普通防御窗口：
 * 窗口开放期间被攻击按普通防御结算（灰色吞格/大破防，见 ResolveGuardHit(false)）。
 * 放到举盾/防御动作蒙太奇上；只切换 Status.Guarding，不碰完美防御窗口。
 * 精准防御（完美窗口/弹反）请用 RH Parry Window。
 */
UCLASS(meta = (DisplayName = "RH Guard Window"))
class DEMO_API URHGuardWindowNotifyState : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};
