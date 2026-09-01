#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "KnsSuperArmorNotifyState.generated.h"

/**
 * Kns Super Armor（霸体窗口）
 * 所属系统：KNS（MH 遗留）
 * 驱动组件：UKnsComboComponent::SetSuperArmor
 * 用途：窗口内受击不被打断（霸体）
 * 放置：霸体招式段
 * 参数：无
 */
UCLASS(meta = (DisplayName = "Kns Super Armor"))
class DEMO_API UKnsSuperArmorNotifyState : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;
};
