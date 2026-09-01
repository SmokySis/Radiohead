#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "RHReverseJustReloadWindowNotifyState.generated.h"

/**
 * RH Reverse Just Reload Window（精准逆转逆装填窗口）：
 * 窗口开放时立即消耗共鸣（沉没成本），消耗时长由 ConsumeResonanceSeconds 配置；
 * 窗口开放期间被攻击 → 按预消耗记录的共鸣类型反色生成手牌音形（红→蓝、蓝→红、平→平），
 * 并让本次命中无效化。无共鸣时窗口直接不生效（命中照常结算，不无敌）。
 */
UCLASS(meta = (DisplayName = "RH Reverse Just Reload Window"))
class DEMO_API URHReverseJustReloadWindowNotifyState : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	/** 窗口打开时消耗的共鸣时长（秒）：默认 20（远大于持续时间=直接用完）；配一半等数值做部分消耗。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReverseJustReload", meta = (ClampMin = "0"))
	float ConsumeResonanceSeconds = 20.f;

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};
