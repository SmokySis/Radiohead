#include "RHEnemyStateTreeEvaluator.h"

#include "Demo/AI/RHEnemyAIComponent.h"
#include "Demo/AI/RHEnemyAIHelpers.h"
#include "Demo/Character/RHEnemyBase.h"
#include "AIController.h"
#include "HAL/PlatformTime.h"
#include "StateTreeExecutionContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RHEnemyStateTreeEvaluator)

namespace
{
	double& GetLastContextLogTime()
	{
		static double Last = -1.0;
		return Last;
	}

	bool ShouldLogContext(double IntervalSeconds)
	{
		const double Now = FPlatformTime::Seconds();
		if (GetLastContextLogTime() < 0.0 || Now - GetLastContextLogTime() >= IntervalSeconds)
		{
			GetLastContextLogTime() = Now;
			return true;
		}
		return false;
	}
}

void FRHEnemyContextEvaluator::TreeStart(FStateTreeExecutionContext& Context) const
{
	UE_LOG(LogTemp, Log, TEXT("[EnemyContext] TreeStart"));
	Tick(Context, 0.f);
}

void FRHEnemyContextEvaluator::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	ARHEnemyBase* Pawn = RHEnemyAI::GetEnemyFromController(Data.AIController);
	Data.EnemyPawn = Pawn;
	Data.PlayerTarget = nullptr;
	Data.Snapshot = FRHPlayerCombatSnapshot();
	Data.CounterBarPercent = 0.f;
	Data.bCounterBarEmpty = false;
	Data.bResonanceBroken = false;
	Data.bIsDowned = false;
	Data.CurrentPhaseIndex = 1;
	Data.ResonancePercent = 0.f;

	if (!Pawn)
	{
		if (ShouldLogContext(1.0))
		{
			UE_LOG(LogTemp, Warning, TEXT("[EnemyContext] FAIL Controller=%s Pawn=NULL (binding/boss mismatch?)"), *GetNameSafe(Data.AIController.Get()));
		}
		return;
	}

	URHEnemyAIComponent* AI = Pawn->FindComponentByClass<URHEnemyAIComponent>();
	if (!AI)
	{
		if (ShouldLogContext(1.0))
		{
			UE_LOG(LogTemp, Warning, TEXT("[EnemyContext] FAIL Pawn=%s has no RHEnemyAIComponent"), *GetNameSafe(Pawn));
		}
		return;
	}

	AI->RefreshSnapshot();
	Data.PlayerTarget = AI->GetPlayerTarget();
	Data.Snapshot = AI->GetSnapshot();
	Data.CounterBarPercent = AI->GetCounterBarPercent();
	Data.bCounterBarEmpty = AI->IsCounterBarEmpty();
	Data.bResonanceBroken = AI->IsBroken();
	Data.bIsDowned = AI->IsDowned();
	Data.CurrentPhaseIndex = AI->GetCurrentPhase();
	Data.ResonancePercent = AI->GetResonancePercent();

	if (ShouldLogContext(1.0))
	{
		AI->DebugPrint(FString::Printf(
			TEXT("Context Player=%s Dist=%.1f Dist2D=%.1f Intent=%s Busy=%d Attacking=%d Broken=%d Downed=%d Phase=%d"),
			*GetNameSafe(Data.PlayerTarget),
			Data.Snapshot.Distance,
			Data.Snapshot.Distance2D,
			*Data.Snapshot.PlayerIntent.ToString(),
			Data.Snapshot.bPlayerBusy ? 1 : 0,
			Data.Snapshot.bPlayerAttacking ? 1 : 0,
			Data.bResonanceBroken ? 1 : 0,
			Data.bIsDowned ? 1 : 0,
			Data.CurrentPhaseIndex));
	}
}
