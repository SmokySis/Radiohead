#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "RHTrailNotifyState.generated.h"

/**
 * RH Weapon Trail（武器拖尾窗口）：
 * 已由代码全权管理（动作释放时自动激活、动作结束/取消/被打断时关闭），本 ANS 不再控制拖尾，
 * 保留仅为兼容存量蒙太奇上的通知（删除会导致资产引用失效）。
 * 拖尾资产：武器基类上的常驻 Niagara 组件（WeaponTrail，资产/位置/变换由用户在组件上配置）。
 */
UCLASS(meta = (DisplayName = "RH Weapon Trail"))
class DEMO_API URHTrailNotifyState : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};
