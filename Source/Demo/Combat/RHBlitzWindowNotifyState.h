#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "GameplayTagContainer.h"
#include "RHBlitzWindowNotifyState.generated.h"

/**
 * RH Blitz Window（闪击窗口）：
 * 开启期间玩家骨骼网格体（不是胶囊体）接触到的角色，视同被武器命中框打中（ReportHit 结算）。
 * 同时开启一段武器命中框（HitboxTag）。
 */
UCLASS(meta = (DisplayName = "RH Blitz Window"))
class DEMO_API URHBlitzWindowNotifyState : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	/** 闪击窗口同时开启的武器命中框标识。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blitz")
	FGameplayTag HitboxTag;

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};
