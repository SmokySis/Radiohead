#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "RHTargetLockInterface.generated.h"

UINTERFACE(MinimalAPI)
class URHTargetLockInterface : public UInterface
{
	GENERATED_BODY()
};

class DEMO_API IRHTargetLockInterface
{
	GENERATED_BODY()

public:
	/** 锁定目标变化通知：true=锁定，false=解除锁定。 */
	virtual void NotifyTargetLockChanged(bool bLocked) = 0;
};
