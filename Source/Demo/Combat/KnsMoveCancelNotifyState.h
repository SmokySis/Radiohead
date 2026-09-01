#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "KnsMoveCancelNotifyState.generated.h"

/**
 * Kns Move Cancel（移动取消窗口）
 * 所属系统：KNS（MH 遗留）
 * 驱动组件：ABaseCharacter（NotifyTick 检测 IA_Move，有输入即打断当前蒙太奇）
 * 用途：窗口内出现移动输入会取消当前招式
 * 放置：允许被移动取消的招式段
 * 参数：无
 */
UCLASS(meta = (DisplayName = "Kns Move Cancel"))
class DEMO_API UKnsMoveCancelNotifyState : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;
};
