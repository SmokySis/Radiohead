#include "RHEnemyDefinition.h"

#include "Demo/AI/RHEnemyMoveDefinition.h"
#include "Demo/AI/RHEnemyMovePoolDefinition.h"

void URHEnemyDefinition::GetResolvedConfig(int32 PhaseIndex, FRHEnemyRuntimeConfig& OutConfig) const
{
	OutConfig.Attributes = Attributes;
	OutConfig.CounterBar = CounterBar;
	OutConfig.Resonance = Resonance;
	OutConfig.Feel = Feel;
	OutConfig.Openers.Reset();
	OutConfig.MiddleMoves.Reset();
	OutConfig.Finishers.Reset();
	OutConfig.SpecialMoves.Reset();
	OutConfig.RemoteMoves.Reset();
	if (MeleeMovePool)
	{
		OutConfig.Openers = MeleeMovePool->Openers;
		OutConfig.MiddleMoves = MeleeMovePool->MiddleMoves;
		OutConfig.Finishers = MeleeMovePool->Finishers;
		OutConfig.SpecialMoves = MeleeMovePool->SpecialMoves;
		OutConfig.ComboEndSpecialChance = MeleeMovePool->ComboEndSpecialChance;
	}
	OutConfig.RemoteMoves = RemoteMoves;
	OutConfig.RemoteAttackThreshold = RemoteAttackThreshold;
	OutConfig.RemoteCooldownSeconds = RemoteCooldownSeconds;
	OutConfig.bFloatBar = bFloatBar;
	OutConfig.DeflectMontage = DeflectMontage;
	OutConfig.DownMontage = DownMontage;
	OutConfig.GetupMontage = GetupMontage;
	OutConfig.DeathMontage = DeathMontage;
	OutConfig.DodgeMontage = DodgeMontage;
	OutConfig.GuardMontage = GuardMontage;

	// phase count 由 Phases 数组顺序决定：Phases[0] = phase 2、Phases[1] = phase 3 ……，开局默认 phase 1（基础）。
	if (Phases.IsValidIndex(PhaseIndex - 2))
	{
		const FRHEnemyPhaseConfig& Phase = Phases[PhaseIndex - 2];
		if (Phase.bOverrideAttributes)
		{
			OutConfig.Attributes = Phase.Attributes;
		}
		if (Phase.bOverrideCounterBar)
		{
			OutConfig.CounterBar = Phase.CounterBar;
		}
		if (Phase.bOverrideResonance)
		{
			OutConfig.Resonance = Phase.Resonance;
		}
		if (Phase.bOverrideFeel)
		{
			OutConfig.Feel = Phase.Feel;
		}
		if (Phase.bOverrideMeleeMoves)
		{
			OutConfig.Openers = Phase.Openers;
			OutConfig.MiddleMoves = Phase.MiddleMoves;
			OutConfig.Finishers = Phase.Finishers;
		}
		if (Phase.bOverrideSpecialMoves)
		{
			OutConfig.SpecialMoves = Phase.SpecialMoves;
		}
		if (!Phase.EntranceMontage.IsNull())
		{
			OutConfig.EntranceMontage = Phase.EntranceMontage;
		}
	}
}

FPrimaryAssetId URHEnemyDefinition::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("RHEnemyDefinition"), EnemyId);
}
