#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "RHGetupNotify.generated.h"

/**
 * RH Getup（起身点，时间点 Notify）。
 * 放在倒地蒙太奇的"完全站起"处：调用战斗组件 EndKnockdown，
 * 移除 Status.Invincible / Status.KnockedDown，结束倒地无敌保护。
 */
UCLASS(meta = (DisplayName = "RH Getup"))
class DEMO_API URHGetupNotify : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};
