#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "KnsTouchConditionNotifyState.generated.h"

/**
 * Kns Touch Condition（实体接触条件窗口）
 * 所属系统：KNS（MH 遗留）
 * 驱动组件：UKnsCombatComponent::SetTouchConditionEnabled
 * 用途：窗口内启用实体重叠接触检测（招式判定的一种）
 * 放置：需要接触判定的招式段
 * 参数：无
 */
UCLASS(meta = (DisplayName = "Kns Touch Condition"))
class DEMO_API UKnsTouchConditionNotifyState : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;
};
