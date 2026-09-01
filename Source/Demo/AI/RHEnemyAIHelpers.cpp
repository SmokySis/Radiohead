#include "RHEnemyAIHelpers.h"

#include "AIController.h"
#include "Demo/AI/RHEnemyAIComponent.h"
#include "Demo/AI/RHEnemyCombatComponent.h"
#include "Demo/Character/RHEnemyBase.h"

ARHEnemyBase* RHEnemyAI::GetEnemyFromController(const AAIController* Controller)
{
	return Controller ? Cast<ARHEnemyBase>(Controller->GetPawn()) : nullptr;
}

URHEnemyAIComponent* RHEnemyAI::GetAIComponent(const AAIController* Controller)
{
	ARHEnemyBase* Enemy = GetEnemyFromController(Controller);
	return Enemy ? Enemy->FindComponentByClass<URHEnemyAIComponent>() : nullptr;
}

URHEnemyCombatComponent* RHEnemyAI::GetCombatComponent(const AAIController* Controller)
{
	ARHEnemyBase* Enemy = GetEnemyFromController(Controller);
	return Enemy ? Enemy->FindComponentByClass<URHEnemyCombatComponent>() : nullptr;
}
