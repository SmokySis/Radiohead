#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "RHReloadNotify.generated.h"

/**
 * RH Reload（逆向装填，时间点 Notify）：
 * 将共鸣槽按等级和类型反向给回特定数目的 onom——不覆盖现有非灰音形，
 * 但可覆盖灰色音形，剩余补空位。
 * 例：手牌[蓝,灰,空] + 3 级红共鸣 → [蓝,红,红]。
 */
UCLASS(meta = (DisplayName = "RH Reload"))
class DEMO_API URHReloadNotify : public UAnimNotify
{
	GENERATED_BODY()

public:
	/** 消耗的共鸣时长（秒）：默认 20（远大于共鸣持续时间，等价于直接用完）；配为一半等数值可做部分消耗。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reload", meta = (ClampMin = "0"))
	float ConsumeResonanceSeconds = 20.f;

	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};
