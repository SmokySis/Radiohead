#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "RHCombatActionInterface.generated.h"

class AActor;

UINTERFACE(MinimalAPI)
class URHCombatActionInterface : public UInterface
{
	GENERATED_BODY()
};

class DEMO_API IRHCombatActionInterface
{
	GENERATED_BODY()

public:
	virtual void SetComboWindowOpen(bool bOpen) = 0;
	virtual void SetPreInputWindowOpen(bool bOpen) = 0;
	virtual bool HasPendingAction() const = 0;
	virtual bool CanConsumeOnom(int32 Amount) const = 0;
	virtual bool TryConsumeOnom(int32 Amount) = 0;
	virtual void StartExecution(AActor* Enemy) = 0;
	virtual void HandleDeflected(AActor* Enemy) = 0;
	virtual void NotifyEnemyDefeated(AActor* Enemy) = 0;
	virtual void SetExecutionAvailable(bool bAvailable, AActor* Enemy) = 0;
	virtual bool TryStartExecution() = 0;
};
