#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "GameplayTagContainer.h"
#include "RHClashWindowNotifyState.generated.h"

/**
 * RH Clash Window（相杀窗口）：
 * 开启武器命中框；若与其它武器的命中框相撞则结算相杀——双方命中框完成判定、互不造成伤害，
 * 玩家先获得“命中”Onom（AttackHitRule），0.1s 后再获得“完美防御”Onom（PerfectGuardHitRule）。
 */
UCLASS(meta = (DisplayName = "RH Clash Window"))
class DEMO_API URHClashWindowNotifyState : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	/** 相杀窗口同时开启的武器命中框标识。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clash")
	FGameplayTag HitboxTag;

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};
