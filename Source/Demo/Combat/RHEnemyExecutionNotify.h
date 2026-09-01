#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "RHEnemyExecutionNotify.generated.h"

/**
 * Single-point notify placed on the enemy execution montage.
 * Applies the fixed execution damage without going through hit reactions or AI.HitTaken.
 */
UCLASS(meta = (DisplayName = "Enemy Execution Damage"))
class DEMO_API URHEnemyExecutionNotify : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;
};
