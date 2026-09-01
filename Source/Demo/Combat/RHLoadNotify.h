#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "RHLoadNotify.generated.h"

/**
 * RH Load（装填执行点，时间点 Notify）：
 * 装填/抛弹统一执行点：按当前手牌自动分流——有灰色音形→抛弹（清手牌），无灰色→装填（存入共鸣）。
 * 装填动画和抛弹动画都用这个 AN（原 RH Toss 已删除）。
 */
UCLASS(meta = (DisplayName = "RH Load"))
class DEMO_API URHLoadNotify : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};
