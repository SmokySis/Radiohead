#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "RHJustReloadWindowNotifyState.generated.h"

/**
 * RH Just Reload Window（精准逆装填窗口）：
 * 窗口开放期间被攻击 → 立即执行逆装填（共鸣按等级/类型反向生成手牌音形，消耗时长可自定义），
 * 并让本次命中无效化。无共鸣槽时窗口空转（命中照常结算，不无敌）。
 */
UCLASS(meta = (DisplayName = "RH Just Reload Window"))
class DEMO_API URHJustReloadWindowNotifyState : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	/** 消耗的共鸣时长（秒）：默认 20（远大于持续时间=直接用完）；配一半等数值做部分消耗。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JustReload", meta = (ClampMin = "0"))
	float ConsumeResonanceSeconds = 20.f;

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};
