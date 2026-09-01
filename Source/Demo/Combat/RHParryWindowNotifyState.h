#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "RHParryWindowNotifyState.generated.h"

/**
 * RH Parry Window（精准防御/弹反窗口）：
 * 窗口开放期间被攻击按完美防御结算（小调音形等，见 ResolveGuardHit(true)）。
 * 与 RH Guard Window（普通防御）分开摆放，通常放在防御/格挡蒙太奇的起手帧。
 */
UCLASS(meta = (DisplayName = "RH Parry Window"))
class DEMO_API URHParryWindowNotifyState : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	/** >0 时覆盖通知自身时长；0（默认）使用该通知段的时长作为窗口时长。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parry", meta = (ClampMin = "0"))
	float WindowSeconds = 0.f;

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};
