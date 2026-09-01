#include "RHEnemyStateTreeConditions.h"

#include "Demo/AI/RHEnemyAIComponent.h"
#include "Demo/AI/RHEnemyAIHelpers.h"
#include "Demo/GAS/KnsAbilitySystemComponent.h"
#include "Demo/GAS/KnsCommonAttributeSet.h"
#include "AIController.h"
#include "HAL/PlatformTime.h"
#include "StateTreeExecutionContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RHEnemyStateTreeConditions)

namespace
{
	double& GetLastConditionLogTime()
	{
		static double Last = -1.0;
		return Last;
	}

	bool ShouldLogCondition(double IntervalSeconds)
	{
		const double Now = FPlatformTime::Seconds();
		if (GetLastConditionLogTime() < 0.0 || Now - GetLastConditionLogTime() >= IntervalSeconds)
		{
			GetLastConditionLogTime() = Now;
			return true;
		}
		return false;
	}
}

bool FRHEnemyDistanceCondition::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& Data = Context.GetInstanceData(*this);
	if (!Data.AIController)
	{
		if (ShouldLogCondition(1.0))
		{
			UE_LOG(LogTemp, Warning, TEXT("[EnemyDistanceCondition] FAIL AIController=NULL (condition binding missing)"));
		}
		return false;
	}
	URHEnemyAIComponent* AI = RHEnemyAI::GetAIComponent(Data.AIController);
	if (!AI)
	{
		if (ShouldLogCondition(1.0))
		{
			UE_LOG(LogTemp, Warning, TEXT("[EnemyDistanceCondition] FAIL Controller=%s has no RHEnemyAIComponent"), *GetNameSafe(Data.AIController.Get()));
		}
		return false;
	}

	const FRHPlayerCombatSnapshot& Snap = AI->GetSnapshot();
	const float Distance = Data.bUse2D ? Snap.Distance2D : Snap.Distance;
	if (Data.MinDistance > 0.f && Distance < Data.MinDistance)
	{
		if (ShouldLogCondition(1.0))
		{
			AI->DebugPrint(FString::Printf(
				TEXT("[DistanceCond] FAIL distance=%.1f < MinDistance=%.1f (use2D=%d)"),
				Distance, Data.MinDistance, Data.bUse2D ? 1 : 0));
		}
		return false;
	}
	if (Data.MaxDistance > 0.f && Distance > Data.MaxDistance)
	{
		if (ShouldLogCondition(1.0))
		{
			AI->DebugPrint(FString::Printf(
				TEXT("[DistanceCond] FAIL distance=%.1f > MaxDistance=%.1f (use2D=%d)"),
				Distance, Data.MaxDistance, Data.bUse2D ? 1 : 0));
		}
		return false;
	}
	if (ShouldLogCondition(1.0))
	{
		AI->DebugPrint(FString::Printf(
			TEXT("[DistanceCond] PASS distance=%.1f min=%.1f max=%.1f (use2D=%d)"),
			Distance, Data.MinDistance, Data.MaxDistance, Data.bUse2D ? 1 : 0));
	}
	return true;
}

bool FRHEnemyPlayerStateCondition::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& Data = Context.GetInstanceData(*this);
	if (!Data.AIController)
	{
		if (ShouldLogCondition(1.0))
		{
			UE_LOG(LogTemp, Warning, TEXT("[EnemyPlayerIntentCondition] FAIL AIController=NULL"));
		}
		return false;
	}
	URHEnemyAIComponent* AI = RHEnemyAI::GetAIComponent(Data.AIController);
	if (!AI)
	{
		if (ShouldLogCondition(1.0))
		{
			UE_LOG(LogTemp, Warning, TEXT("[EnemyPlayerIntentCondition] FAIL Controller=%s has no RHEnemyAIComponent"), *GetNameSafe(Data.AIController.Get()));
		}
		return false;
	}

	const FRHPlayerCombatSnapshot& Snap = AI->GetSnapshot();
	if (Data.PlayerIntent.IsValid() && !Snap.PlayerIntent.MatchesTag(Data.PlayerIntent))
	{
		if (ShouldLogCondition(1.0))
		{
			AI->DebugPrint(FString::Printf(
				TEXT("[PlayerIntentCond] FAIL required=%s actual=%s"),
				*Data.PlayerIntent.ToString(),
				*Snap.PlayerIntent.ToString()));
		}
		return false;
	}
	return true;
}

bool FRHEnemyResourceCondition::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& Data = Context.GetInstanceData(*this);
	if (!Data.AIController)
	{
		if (ShouldLogCondition(1.0))
		{
			UE_LOG(LogTemp, Warning, TEXT("[EnemyResourceCondition] FAIL AIController=NULL"));
		}
		return false;
	}
	URHEnemyAIComponent* AI = RHEnemyAI::GetAIComponent(Data.AIController);
	if (!AI)
	{
		if (ShouldLogCondition(1.0))
		{
			UE_LOG(LogTemp, Warning, TEXT("[EnemyResourceCondition] FAIL Controller=%s has no RHEnemyAIComponent"), *GetNameSafe(Data.AIController.Get()));
		}
		return false;
	}

	bool bResult = false;
	switch (Data.Test)
	{
	case ERHEnemyResourceTest::CounterBarEmpty:
		bResult = AI->IsCounterBarEmpty();
		break;
	case ERHEnemyResourceTest::ResonanceBroken:
		bResult = AI->IsBroken();
		break;
	case ERHEnemyResourceTest::PhaseAtLeast:
		bResult = AI->GetCurrentPhase() >= Data.Phase;
		break;
	case ERHEnemyResourceTest::PhaseAtMost:
		bResult = AI->GetCurrentPhase() <= Data.Phase;
		break;
	case ERHEnemyResourceTest::MoveSetAvailable:
		bResult = AI->HasMoveSet();
		break;
	case ERHEnemyResourceTest::SpecialAvailable:
		bResult = AI->IsSpecialAvailable();
		break;
	case ERHEnemyResourceTest::DodgeCooldownReady:
		bResult = AI->CanDodge();
		break;
	case ERHEnemyResourceTest::IsDead:
	{
		if (UKnsAbilitySystemComponent* ASC = AI->GetEnemyASC())
		{
			bResult = ASC->GetAttributeValue(UKnsCommonAttributeSet::GetHealthAttribute()) <= 0.f;
		}
		break;
	}
	case ERHEnemyResourceTest::IsAttacking:
		bResult = AI->IsAttacking();
		break;
	case ERHEnemyResourceTest::HealthPercentBelow:
	{
		if (UKnsAbilitySystemComponent* ASC = AI->GetEnemyASC())
		{
			const float Max = ASC->GetAttributeValue(UKnsCommonAttributeSet::GetMaxHealthAttribute());
			const float Current = ASC->GetAttributeValue(UKnsCommonAttributeSet::GetHealthAttribute());
			bResult = Max > 0.f && (Current / Max) * 100.f <= Data.HealthPercent;
		}
		break;
	}
	case ERHEnemyResourceTest::PlayerHealthPercentBelow:
		bResult = AI->GetSnapshot().PlayerHealthPercent <= Data.PlayerHealthPercent;
		break;
	case ERHEnemyResourceTest::PlayerWeaponIdEquals:
		bResult = AI->GetSnapshot().PlayerWeaponId == Data.PlayerWeaponId;
		break;
	default:
		bResult = false;
		break;
	}

	if (ShouldLogCondition(1.0))
	{
		AI->DebugPrint(FString::Printf(
			TEXT("[ResourceCond] Test=%d Phase=%d MoveSet=%d Special=%d -> %s"),
			static_cast<int32>(Data.Test),
			Data.Phase,
			AI->HasMoveSet() ? 1 : 0,
			AI->IsSpecialAvailable() ? 1 : 0,
			bResult ? TEXT("PASS") : TEXT("FAIL")));
	}
	return bResult;
}
