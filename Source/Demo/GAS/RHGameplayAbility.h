#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "RHGameplayAbility.generated.h"

UCLASS()
class DEMO_API URHGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Onom", meta = (ClampMin = "0"))
	int32 OnomCost = 1;
};
