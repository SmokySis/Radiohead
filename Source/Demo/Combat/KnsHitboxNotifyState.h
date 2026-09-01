#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "GameplayTagContainer.h"
#include "KnsHitboxNotifyState.generated.h"

/**
 * Kns Hitbox（命中框窗口）
 * 所属系统：KNS（MH 遗留）
 * 驱动组件：UKnsCombatComponent::BeginHitbox / EndHitbox
 * 用途：开放/关闭一段命中判定框（重叠检测 + ReportHit）
 * 放置：攻击蒙太奇的命中段
 * 参数：HitboxTag（该段命中框的标识 Tag）
 */
UCLASS(meta = (DisplayName = "Kns Hitbox"))
class DEMO_API UKnsHitboxNotifyState : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	// 这段命中框的标识 Tag
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hitbox", meta = (ToolTip = "这段命中框的标识 Tag"))
	FGameplayTag HitboxTag;

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;
};
