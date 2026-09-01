#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "KnsRollCancelNotifyState.generated.h"

/**
 * Kns Roll Cancel（翻滚取消窗口）
 * 所属系统：KNS（MH 遗留）
 * 驱动组件：ASC（Combo.Cancel.Roll 标签）
 * 用途：窗口内允许翻滚取消当前招式
 * 放置：攻击蒙太奇的翻滚取消段
 * 参数：无
 */
UCLASS(meta = (DisplayName = "Kns Roll Cancel"))
class DEMO_API UKnsRollCancelNotifyState : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;
};
