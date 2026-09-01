#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "RHJustLoadNotify.generated.h"

/**
 * RH Just Load（时间点 Notify）：
 * 不播放蒙太奇，直接触发 load——有灰色音形=抛弹（清手牌）、无灰色=装填（存入共鸣），
 * 播放 load 音效并执行其效果。默认不播 VFX（bPlayVFX=true 时在角色位置播 parry 同款反馈）。
 */
UCLASS(meta = (DisplayName = "RH Just Load"))
class DEMO_API URHJustLoadNotify : public UAnimNotify
{
	GENERATED_BODY()

public:
	/** 是否播放 parry 同款 VFX（polarity=Neutral）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JustLoad")
	bool bPlayVFX = false;

	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};
