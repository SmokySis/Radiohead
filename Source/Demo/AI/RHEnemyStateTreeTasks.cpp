#include "RHEnemyStateTreeTasks.h"

#include "AIController.h"
#include "Animation/AnimMontage.h"
#include "Demo/AI/RHEnemyAIController.h"
#include "Demo/AI/RHEnemyAIComponent.h"
#include "Demo/AI/RHEnemyActionDefinition.h"
#include "Demo/AI/RHEnemyAIHelpers.h"
#include "Demo/AI/RHEnemyCombatComponent.h"
#include "Demo/AI/RHEnemyMoveDefinition.h"
#include "Demo/Character/RHEnemyBase.h"
#include "Demo/Combat/KnsCombatComponent.h"
#include "Demo/Combat/RHCombatActionInterface.h"
#include "Demo/GAS/KnsAbilitySystemComponent.h"
#include "Demo/Onom/RHWeaponDefinition.h"
#include "Engine/World.h"
#include "Engine/EngineTypes.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HAL/PlatformTime.h"
#include "Navigation/PathFollowingComponent.h"
#include "StateTreeExecutionContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RHEnemyStateTreeTasks)

namespace
{
	// Aggressive 连段阶段（FRHEnemyAggressiveTaskInstanceData::ComboPhase）。
	constexpr uint8 EComboPhase_None = 0;
	constexpr uint8 EComboPhase_Opener = 1;
	constexpr uint8 EComboPhase_Middle = 2;
	constexpr uint8 EComboPhase_Finisher = 3;
	constexpr uint8 EComboPhase_Special = 4;
	constexpr uint8 EComboPhase_Remote = 5;

	void LogEnter(const URHEnemyAIComponent* AI, const FString& Message)
	{
		if (AI)
		{
			AI->DebugPrint(Message);
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("[AI] %s"), *Message);
		}
	}

	void LogTick(const URHEnemyAIComponent* AI, double& LastLogTime, const FString& Message)
	{
		if (!AI)
		{
			return;
		}
		const double Now = FPlatformTime::Seconds();
		if (LastLogTime < 0.0 || Now - LastLogTime >= 1.0)
		{
			LastLogTime = Now;
			AI->DebugPrint(Message);
		}
	}

	void SetEnemyWalkSpeed(ARHEnemyBase* Pawn, float Speed)
	{
		if (Pawn && Pawn->GetCharacterMovement())
		{
			Pawn->GetCharacterMovement()->MaxWalkSpeed = Speed;
		}
	}

	// Idle / Guard 共用的自由走动：永不站立，左走/右走/后退循环（后退最多 1~2s）。
	void TickWander(ARHEnemyBase* Pawn, URHEnemyAIComponent* AI, FRHEnemyWanderState& State, float DeltaTime)
	{
		if (!Pawn || !AI)
		{
			return;
		}

		const FRHEnemyFeelConfig& Feel = AI->GetCurrentConfig().Feel;
		State.MoveTimer -= DeltaTime;
		if (State.MoveTimer <= 0.f)
		{
			const int32 Roll = FMath::RandRange(0, 99);
			if (Roll < 45)
			{
				State.MoveMode = 0; // 左走
				State.MoveTimer = Feel.IdleStrafeDuration;
			}
			else if (Roll < 90)
			{
				State.MoveMode = 1; // 右走
				State.MoveTimer = Feel.IdleStrafeDuration;
			}
			else
			{
				State.MoveMode = 2; // 后退，最多 1~2s
				State.MoveTimer = FMath::FRandRange(1.f, 2.f);
			}
		}

		SetEnemyWalkSpeed(Pawn, Feel.IdleStrafeSpeed);
		FVector Dir = FVector::ZeroVector;
		if (State.MoveMode == 0)
		{
			Dir = -Pawn->GetActorRightVector();
		}
		else if (State.MoveMode == 1)
		{
			Dir = Pawn->GetActorRightVector();
		}
		else
		{
			Dir = -Pawn->GetActorForwardVector();
		}
		Pawn->AddMovementInput(Dir);
	}

	// 受击硬直 / 破防 / 受击动画中：行为任务一律中止（被打断，且不被其他行为接走）。
	bool ShouldAbortForStagger(const URHEnemyAIComponent* AI, const URHEnemyCombatComponent* Combat)
	{
		return AI && (AI->IsBroken() || AI->IsStaggered() || (Combat && Combat->IsHitReactionPlaying()));
	}

	// 战斗动作（Attack/Special/Guard/Dodge/Approach）在弹开时也要中止；
	// 但 Idle 等被动状态不中止，避免 Deflect 期间 Idle 反复 FAIL。
	bool ShouldAbortForDeflect(const URHEnemyAIComponent* AI, const URHEnemyCombatComponent* Combat)
	{
		return ShouldAbortForStagger(AI, Combat) || (AI && AI->IsDeflecting());
	}

	// Deflect 任务本身要在弹开期间运行，所以不把 IsDeflecting 视为需要中止。
	bool ShouldAbortForStaggerExcludingDeflect(const URHEnemyAIComponent* AI, const URHEnemyCombatComponent* Combat)
	{
		return AI && (AI->IsBroken() || AI->IsStaggered() || (Combat && Combat->IsHitReactionPlaying()));
	}
}

// ---------------------------------------------------------------------------
// Approach
// ---------------------------------------------------------------------------
EStateTreeRunStatus FRHEnemyApproachTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	ARHEnemyBase* Pawn = RHEnemyAI::GetEnemyFromController(Data.AIController);
	URHEnemyAIComponent* AI = RHEnemyAI::GetAIComponent(Data.AIController);
	if (!Data.AIController || !Pawn || !AI)
	{
		LogEnter(AI, TEXT("[Approach] ENTER FAIL Controller/Pawn/AI missing"));
		return EStateTreeRunStatus::Failed;
	}

	if (ShouldAbortForDeflect(AI, RHEnemyAI::GetCombatComponent(Data.AIController)))
	{
		LogEnter(AI, TEXT("[Approach] ENTER FAIL staggered/broken/hitreaction"));
		return EStateTreeRunStatus::Failed;
	}

	AActor* Player = AI->GetPlayerTarget();
	if (!Player)
	{
		LogEnter(AI, TEXT("[Approach] ENTER FAIL player target missing"));
		return EStateTreeRunStatus::Failed;
	}

	Data.LastMoveRequestTime = -1.f;
	Data.bUsingDirectMovement = false;
	Data.bForwardDodgeDone = false;
	Data.Elapsed = 0.f;
	Data.TimeElapsed = 0.f;
	AI->SetRotateToPlayer(true);

	// Approach 用战斗走速（600，由 DA WalkSpeed 控制）。
	SetEnemyWalkSpeed(Pawn, AI->GetCurrentConfig().Attributes.WalkSpeed);

	URHEnemyCombatComponent* Combat = RHEnemyAI::GetCombatComponent(Data.AIController);
	const float Dist2D = (Player->GetActorLocation() - Pawn->GetActorLocation()).Size2D();
	if (Dist2D <= Data.AcceptanceRadius + Data.AcceptanceTolerance)
	{
		LogEnter(AI, FString::Printf(TEXT("[Approach] ENTER SUCCEED already in range dist2D=%.1f threshold=%.1f"), Dist2D, Data.AcceptanceRadius + Data.AcceptanceTolerance));
		return EStateTreeRunStatus::Succeeded;
	}
	if (Dist2D > AI->GetCurrentConfig().Feel.ApproachDodgeFirstThreshold)
	{
		// 距离特别远：先向前突进一次（闪避蒙太奇 F 段），再奔跑接近。每轮 Approach 只突进一次。
		UAnimMontage* DodgeMontage = AI->GetCurrentConfig().DodgeMontage.LoadSynchronous();
		if (Combat && DodgeMontage)
		{
			Combat->PlayMontageSection(DodgeMontage, TEXT("F"));
		}
		else
		{
			Pawn->LaunchCharacter(Pawn->GetActorForwardVector() * 600.f, false, false);
		}
		Data.bForwardDodgeDone = true;
		LogEnter(AI, FString::Printf(TEXT("[Approach] ENTER Running dist2D=%.1f dodgeFirst=1"), Dist2D));
		return EStateTreeRunStatus::Running;
	}

	if (AAIController* Controller = Pawn->GetController<AAIController>())
	{
		Controller->MoveToActor(Player, Data.AcceptanceRadius + Data.AcceptanceTolerance, true, true, true, nullptr, true);
		Data.LastMoveRequestTime = Context.GetWorld() ? Context.GetWorld()->GetTimeSeconds() : 0.f;
	}
	else
	{
		Data.bUsingDirectMovement = true;
	}
	LogEnter(AI, FString::Printf(TEXT("[Approach] ENTER Running dist2D=%.1f acceptance=%.1f"), Dist2D, Data.AcceptanceRadius));
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FRHEnemyApproachTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	ARHEnemyBase* Pawn = RHEnemyAI::GetEnemyFromController(Data.AIController);
	URHEnemyAIComponent* AI = RHEnemyAI::GetAIComponent(Data.AIController);
	if (!Data.AIController || !Pawn || !AI)
	{
		LogEnter(AI, TEXT("[Approach] TICK FAIL Controller/Pawn/AI missing"));
		return EStateTreeRunStatus::Failed;
	}

	if (ShouldAbortForDeflect(AI, RHEnemyAI::GetCombatComponent(Data.AIController)))
	{
		LogEnter(AI, TEXT("[Approach] TICK FAIL staggered/broken/hitreaction"));
		return EStateTreeRunStatus::Failed;
	}

	const FRHPlayerCombatSnapshot& Snap = AI->GetSnapshot();
	// 守武德：玩家喝药时不追击（也不计入追击超时）。
	if (Snap.bPlayerHealing)
	{
		return EStateTreeRunStatus::Running;
	}

	// 内部计时：超过 TimeLimitSeconds 仍未追到玩家 → Failed（0 = 不限制）。
	if (Data.TimeLimitSeconds > 0.f)
	{
		Data.TimeElapsed += DeltaTime;
		if (Data.TimeElapsed >= Data.TimeLimitSeconds)
		{
			AI->DebugPrint(FString::Printf(
				TEXT("[Approach] TICK FAIL time limit %.1fs exceeded"), Data.TimeLimitSeconds));
			return EStateTreeRunStatus::Failed;
		}
	}

	if (Data.bForwardDodgeDone)
	{
		// 突进动画播完再开始正常接近。
		Data.Elapsed += DeltaTime;
		URHEnemyCombatComponent* Combat = RHEnemyAI::GetCombatComponent(Data.AIController);
		if (!(Combat && Combat->IsMontagePlaying()) && Data.Elapsed > 0.15f)
		{
			Data.bForwardDodgeDone = false;
			Data.Elapsed = 0.f;
			if (AAIController* Controller = Pawn->GetController<AAIController>())
			{
				Controller->MoveToActor(Snap.Player, Data.AcceptanceRadius + Data.AcceptanceTolerance, true, true, true, nullptr, true);
				Data.LastMoveRequestTime = Context.GetWorld() ? Context.GetWorld()->GetTimeSeconds() : 0.f;
			}
			else
			{
				Data.bUsingDirectMovement = true;
			}
		}
		return EStateTreeRunStatus::Running;
	}

	if (Snap.Distance2D <= Data.AcceptanceRadius + Data.AcceptanceTolerance)
	{
		AI->DebugPrint(FString::Printf(TEXT("[Approach] SUCCEED dist2D=%.1f radius=%.1f"), Snap.Distance2D, Data.AcceptanceRadius));
		return EStateTreeRunStatus::Succeeded;
	}

	if (Data.bUsingDirectMovement)
	{
		// 无控制器或寻路失败：直接朝玩家直线走（不依赖导航网格）。
		const FVector Dir = (Snap.Player->GetActorLocation() - Pawn->GetActorLocation()).GetSafeNormal2D();
		Pawn->AddMovementInput(Dir);
		return EStateTreeRunStatus::Running;
	}

	if (AAIController* Controller = Pawn->GetController<AAIController>())
	{
		const EPathFollowingStatus::Type MoveStatus = Controller->GetMoveStatus();
		// 路径已走完（到达 navmesh 终点/路径中断/寻路失败）但距离仍超期望：
		// 路径结束后 MoveStatus 都会回到 Idle（PartialPath 是请求结果枚举，不在此列），
		// MoveToActor 已无法再接近（玩家在 navmesh 边缘/目标点不可达），重发只会停在同一个点。
		// 等 0.5s 缓冲（防 MoveToActor 刚发出的瞬态 Idle 误判）后直接转直线移动兜底。
		// 注意：不能在 Idle 时反复重发 MoveToActor —— 重发会刷新 LastMoveRequestTime，
		// 让下面原本"1.5s 转直线"的分支永远等不到，导致原地发呆。
		if (MoveStatus == EPathFollowingStatus::Idle)
		{
			if (UWorld* World = Context.GetWorld())
			{
				const float SinceRequest = World->GetTimeSeconds() - Data.LastMoveRequestTime;
				if (SinceRequest > 0.5f)
				{
					Data.bUsingDirectMovement = true;
				}
			}
		}
	}
	LogTick(AI, Data.LastTickLogTime, FString::Printf(
		TEXT("[Approach] TICK Running dist2D=%.1f acceptance=%.1f direct=%d"),
		Snap.Distance2D, Data.AcceptanceRadius, Data.bUsingDirectMovement ? 1 : 0));
	return EStateTreeRunStatus::Running;
}

void FRHEnemyApproachTask::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	if (Data.AIController)
	{
		Data.AIController->StopMovement();
	}
	if (ARHEnemyBase* Pawn = RHEnemyAI::GetEnemyFromController(Data.AIController))
	{
		if (URHEnemyAIComponent* AI = RHEnemyAI::GetAIComponent(Data.AIController))
		{
			SetEnemyWalkSpeed(Pawn, AI->GetCurrentConfig().Attributes.WalkSpeed);
		}
	}
}

// ---------------------------------------------------------------------------
// Attack
// ---------------------------------------------------------------------------
EStateTreeRunStatus FRHEnemyAttackTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	ARHEnemyBase* Pawn = RHEnemyAI::GetEnemyFromController(Data.AIController);
	URHEnemyAIComponent* AI = RHEnemyAI::GetAIComponent(Data.AIController);
	URHEnemyCombatComponent* Combat = RHEnemyAI::GetCombatComponent(Data.AIController);
	if (!Data.AIController || !Pawn || !AI || !Combat)
	{
		LogEnter(AI, TEXT("[Attack] ENTER FAIL Controller/Pawn/AI/Combat missing"));
		return EStateTreeRunStatus::Failed;
	}

	// 受击硬直 / 破防 / 受击动画中不重开攻击，避免"被打→立刻重播→再被打"的高频打断。
	if (ShouldAbortForDeflect(AI, Combat))
	{
		LogEnter(AI, TEXT("[Attack] ENTER FAIL staggered/broken/hitreaction"));
		return EStateTreeRunStatus::Failed;
	}

	const FRHPlayerCombatSnapshot& Snap = AI->GetSnapshot();
	if (!Snap.Player || Snap.bPlayerHealing)
	{
		LogEnter(AI, TEXT("[Attack] ENTER FAIL no player or player healing"));
		return EStateTreeRunStatus::Failed;
	}

	if (!AI->PickCombo(Data.CurrentChain))
	{
		LogEnter(AI, TEXT("[Attack] ENTER FAIL PickCombo returned no chain"));
		return EStateTreeRunStatus::Failed;
	}
	Data.CurrentStep = 0;
	AI->SetRotateToPlayer(true);

	if (!Combat->PlayMove(Data.CurrentChain[0]))
	{
		LogEnter(AI, FString::Printf(
			TEXT("[Attack] ENTER FAIL PlayMove chain[0]=%s"),
			*GetNameSafe(Data.CurrentChain[0])));
		return EStateTreeRunStatus::Failed;
	}
	LogEnter(AI, FString::Printf(
		TEXT("[Attack] ENTER Running chain=%d first=%s"),
		Data.CurrentChain.Num(), *GetNameSafe(Data.CurrentChain[0])));
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FRHEnemyAttackTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	ARHEnemyBase* Pawn = RHEnemyAI::GetEnemyFromController(Data.AIController);
	URHEnemyAIComponent* AI = RHEnemyAI::GetAIComponent(Data.AIController);
	URHEnemyCombatComponent* Combat = RHEnemyAI::GetCombatComponent(Data.AIController);
	if (!Data.AIController || !Pawn || !AI || !Combat)
	{
		LogEnter(AI, TEXT("[Attack] TICK FAIL Controller/Pawn/AI/Combat missing"));
		return EStateTreeRunStatus::Failed;
	}

	if (ShouldAbortForDeflect(AI, Combat))
	{
		LogEnter(AI, TEXT("[Attack] TICK FAIL staggered/broken/hitreaction"));
		return EStateTreeRunStatus::Failed;
	}

	const FRHPlayerCombatSnapshot& Snap = AI->GetSnapshot();
	// 守武德：玩家喝药时立刻收手。

	if (Snap.bPlayerHealing)
	{
		LogEnter(AI, TEXT("[Attack] TICK FAIL player healing, stopping move"));
		Combat->StopCurrentMoveMontage();
		return EStateTreeRunStatus::Failed;
	}

	if (Combat->IsBusy())
	{
		if (Combat->IsComboWindowOpen() && Data.CurrentChain.IsValidIndex(Data.CurrentStep + 1))
		{
			Data.CurrentStep++;
			if (!Combat->PlayMove(Data.CurrentChain[Data.CurrentStep]))
			{
				LogEnter(AI, FString::Printf(
					TEXT("[Attack] TICK FAIL PlayMove chain[%d]=%s"),
					Data.CurrentStep, *GetNameSafe(Data.CurrentChain[Data.CurrentStep])));
				return EStateTreeRunStatus::Failed;
			}
		}
		LogTick(AI, Data.LastTickLogTime, FString::Printf(
			TEXT("[Attack] TICK Running step=%d/%d busy=1 comboWindow=%d"),
			Data.CurrentStep, Data.CurrentChain.Num(),
			Combat->IsComboWindowOpen() ? 1 : 0));
		return EStateTreeRunStatus::Running;
	}

	// 本段被打断（受击等）：整条连段作废，交给状态树重选。
	if (Combat->WasLastActionInterrupted())
	{
		LogEnter(AI, TEXT("[Attack] TICK FAIL action interrupted"));
		return EStateTreeRunStatus::Failed;
	}

	Data.CurrentStep++;
	if (Data.CurrentChain.IsValidIndex(Data.CurrentStep))
	{
		if (!Combat->PlayMove(Data.CurrentChain[Data.CurrentStep]))
		{
			LogEnter(AI, FString::Printf(
				TEXT("[Attack] TICK FAIL PlayMove chain[%d]=%s"),
				Data.CurrentStep, *GetNameSafe(Data.CurrentChain[Data.CurrentStep])));
			return EStateTreeRunStatus::Failed;
		}
		return EStateTreeRunStatus::Running;
	}

	// 整条连段播完。
	if (Data.bResetCounterBarOnComplete)
	{
		AI->ResetCounterBar();
	}
	LogEnter(AI, TEXT("[Attack] SUCCEED combo complete"));
	return EStateTreeRunStatus::Succeeded;
}

void FRHEnemyAttackTask::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	if (URHEnemyCombatComponent* Combat = RHEnemyAI::GetCombatComponent(Data.AIController))
	{
		Combat->StopCurrentMoveMontage();
	}
}

// ---------------------------------------------------------------------------
// Remote Attack（远程招式：冷却制，冷却中直接 Failed）
// ---------------------------------------------------------------------------
EStateTreeRunStatus FRHEnemyRemoteAttackTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	ARHEnemyBase* Pawn = RHEnemyAI::GetEnemyFromController(Data.AIController);
	URHEnemyAIComponent* AI = RHEnemyAI::GetAIComponent(Data.AIController);
	URHEnemyCombatComponent* Combat = RHEnemyAI::GetCombatComponent(Data.AIController);
	if (!Data.AIController || !Pawn || !AI || !Combat)
	{
		LogEnter(AI, TEXT("[RemoteAttack] ENTER FAIL Controller/Pawn/AI/Combat missing"));
		return EStateTreeRunStatus::Failed;
	}
	if (ShouldAbortForDeflect(AI, Combat))
	{
		LogEnter(AI, TEXT("[RemoteAttack] ENTER FAIL staggered/broken/hitreaction"));
		return EStateTreeRunStatus::Failed;
	}
	const FRHPlayerCombatSnapshot& Snap = AI->GetSnapshot();
	if (!Snap.Player || Snap.bPlayerHealing)
	{
		LogEnter(AI, TEXT("[RemoteAttack] ENTER FAIL no player or player healing"));
		return EStateTreeRunStatus::Failed;
	}

	// 远程冷却：冷却中直接 Failed（交给 approach 近战）。
	if (!AI->CanUseRemote())
	{
		LogEnter(AI, FString::Printf(
			TEXT("[RemoteAttack] ENTER FAIL on cooldown (%.1fs)"), AI->GetRemoteCooldownRemaining()));
		return EStateTreeRunStatus::Failed;
	}

	const FRHEnemyRuntimeConfig& Config = AI->GetCurrentConfig();
	URHEnemyMoveDefinition* RemoteMove = AI->PickRemoteMove(Config.RemoteMoves);
	if (!RemoteMove)
	{
		LogEnter(AI, TEXT("[RemoteAttack] ENTER FAIL no remote moves"));
		return EStateTreeRunStatus::Failed;
	}
	AI->NotifyRemoteUsed();
	AI->SetRotateToPlayer(true);

	if (!Combat->PlayMove(RemoteMove))
	{
		LogEnter(AI, FString::Printf(TEXT("[RemoteAttack] ENTER FAIL PlayMove %s"), *GetNameSafe(RemoteMove)));
		return EStateTreeRunStatus::Failed;
	}
	LogEnter(AI, FString::Printf(TEXT("[RemoteAttack] ENTER Running move=%s"), *GetNameSafe(RemoteMove)));
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FRHEnemyRemoteAttackTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	URHEnemyAIComponent* AI = RHEnemyAI::GetAIComponent(Data.AIController);
	URHEnemyCombatComponent* Combat = RHEnemyAI::GetCombatComponent(Data.AIController);
	if (!Data.AIController || !AI || !Combat)
	{
		LogEnter(AI, TEXT("[RemoteAttack] TICK FAIL Controller/AI/Combat missing"));
		return EStateTreeRunStatus::Failed;
	}
	if (ShouldAbortForDeflect(AI, Combat))
	{
		LogEnter(AI, TEXT("[RemoteAttack] TICK FAIL staggered/broken/hitreaction"));
		return EStateTreeRunStatus::Failed;
	}
	if (AI->GetSnapshot().bPlayerHealing)
	{
		LogEnter(AI, TEXT("[RemoteAttack] TICK FAIL player healing"));
		Combat->StopCurrentMoveMontage();
		return EStateTreeRunStatus::Failed;
	}

	if (Combat->IsBusy())
	{
		return EStateTreeRunStatus::Running;
	}

	// 单段远程招式播完。
	if (Data.bResetCounterBarOnComplete)
	{
		AI->ResetCounterBar();
	}
	LogEnter(AI, TEXT("[RemoteAttack] SUCCEED move complete"));
	return EStateTreeRunStatus::Succeeded;
}

void FRHEnemyRemoteAttackTask::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	if (URHEnemyCombatComponent* Combat = RHEnemyAI::GetCombatComponent(Data.AIController))
	{
		Combat->StopCurrentMoveMontage();
	}
}

// ---------------------------------------------------------------------------
// Aggressive（远程/近战招式池统一挑选与释放）
// ---------------------------------------------------------------------------
EStateTreeRunStatus FRHEnemyAggressiveTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	ARHEnemyBase* Pawn = RHEnemyAI::GetEnemyFromController(Data.AIController);
	URHEnemyAIComponent* AI = RHEnemyAI::GetAIComponent(Data.AIController);
	URHEnemyCombatComponent* Combat = RHEnemyAI::GetCombatComponent(Data.AIController);
	if (!Data.AIController || !Pawn || !AI || !Combat)
	{
		LogEnter(AI, TEXT("[Aggressive] ENTER FAIL Controller/Pawn/AI/Combat missing"));
		return EStateTreeRunStatus::Failed;
	}
	if (ShouldAbortForDeflect(AI, Combat))
	{
		LogEnter(AI, TEXT("[Aggressive] ENTER FAIL staggered/broken/hitreaction"));
		return EStateTreeRunStatus::Failed;
	}
	const FRHPlayerCombatSnapshot& Snap = AI->GetSnapshot();
	if (!Snap.Player || Snap.bPlayerHealing)
	{
		LogEnter(AI, TEXT("[Aggressive] ENTER FAIL no player or player healing"));
		return EStateTreeRunStatus::Failed;
	}

	Data.CurrentStep = 0;
	Data.CurrentChain.Reset();
	Data.bCurrentIsSpecial = false;
	Data.ComboPhase = EComboPhase_None;
	Data.bFinisherPlayed = false;
	// 攻击时强制恢复自动转向：WaitRecover/Down 等任务会关掉旋转，这里兜底（否则受击/弹反后不面向玩家）。
	AI->SetRotateToPlayer(true);

	const FRHEnemyRuntimeConfig& Config = AI->GetCurrentConfig();
	const float Dist = Snap.Distance2D;

	// 远程：达到阈值时若冷却结束 → 随机触发一个远程招式（固定普攻、无特殊）；冷却中 → 交给 approach（Failed）。
	if (Dist >= Config.RemoteAttackThreshold)
	{
		if (AI->CanUseRemote())
		{
			Data.bUsingRemote = true;
		}
		else
		{
			LogEnter(AI, FString::Printf(
				TEXT("[Aggressive] ENTER FAIL remote on cooldown (%.1fs), approach"), AI->GetRemoteCooldownRemaining()));
			return EStateTreeRunStatus::Failed;
		}
	}
	else
	{
		Data.bUsingRemote = false;
	}

	if (Data.bUsingRemote)
	{
		URHEnemyMoveDefinition* RemoteMove = AI->PickRemoteMove(Config.RemoteMoves);
		if (!RemoteMove || !Combat->PlayMove(RemoteMove))
		{
			LogEnter(AI, TEXT("[Aggressive] ENTER FAIL remote move empty or play failed"));
			return EStateTreeRunStatus::Failed;
		}
		AI->NotifyRemoteUsed();
		Data.ComboPhase = EComboPhase_Remote;
		LogEnter(AI, FString::Printf(TEXT("[Aggressive] ENTER Running remote=%s"), *GetNameSafe(RemoteMove)));
		return EStateTreeRunStatus::Running;
	}

	// 近战：禁止特殊招式起手，按距离区间 + 权重选起手式；无符合距离的起手式 → 立刻 Failed。
	URHEnemyMoveDefinition* Opener = AI->PickOpener(Config.Openers);
	if (!Opener)
	{
		LogEnter(AI, TEXT("[Aggressive] ENTER FAIL no opener in range"));
		return EStateTreeRunStatus::Failed;
	}
	if (!Combat->PlayMove(Opener))
	{
		LogEnter(AI, TEXT("[Aggressive] ENTER FAIL PlayMove opener"));
		return EStateTreeRunStatus::Failed;
	}
	Data.ComboPhase = EComboPhase_Opener;
	LogEnter(AI, FString::Printf(TEXT("[Aggressive] ENTER Running opener=%s"), *GetNameSafe(Opener)));
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FRHEnemyAggressiveTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	ARHEnemyBase* Pawn = RHEnemyAI::GetEnemyFromController(Data.AIController);
	URHEnemyAIComponent* AI = RHEnemyAI::GetAIComponent(Data.AIController);
	URHEnemyCombatComponent* Combat = RHEnemyAI::GetCombatComponent(Data.AIController);
	if (!Data.AIController || !Pawn || !AI || !Combat)
	{
		LogEnter(AI, TEXT("[Aggressive] TICK FAIL Controller/Pawn/AI/Combat missing"));
		return EStateTreeRunStatus::Failed;
	}
	if (ShouldAbortForDeflect(AI, Combat))
	{
		LogEnter(AI, TEXT("[Aggressive] TICK FAIL staggered/broken/hitreaction"));
		return EStateTreeRunStatus::Failed;
	}
	if (AI->GetSnapshot().bPlayerHealing)
	{
		LogEnter(AI, TEXT("[Aggressive] TICK FAIL player healing"));
		Combat->StopCurrentMoveMontage();
		return EStateTreeRunStatus::Failed;
	}

	const FRHEnemyRuntimeConfig& Config = AI->GetCurrentConfig();

	// 特殊招式 / 远程单段：等当前动作播完即收尾。
	if (Data.bCurrentIsSpecial || Data.ComboPhase == EComboPhase_Remote)
	{
		if (!Combat->IsBusy())
		{
			AI->ResetCounterBar();
			LogEnter(AI, Data.bCurrentIsSpecial ? TEXT("[Aggressive] SUCCEED special finished") : TEXT("[Aggressive] SUCCEED remote finished"));
			return EStateTreeRunStatus::Succeeded;
		}
		return EStateTreeRunStatus::Running;
	}

	if (Combat->IsBusy())
	{
		// 中间招式段：连段窗口内推进下一段。
		if (Data.ComboPhase == EComboPhase_Middle && Combat->IsComboWindowOpen() && Data.CurrentChain.IsValidIndex(Data.CurrentStep + 1))
		{
			Data.CurrentStep++;
			// 每段攻击开始都重新启用旋转至玩家（上一段蒙太奇里的 Enemy Auto Rotate AN 可能已关掉旋转）。
			AI->SetRotateToPlayer(true);
			if (!Combat->PlayMove(Data.CurrentChain[Data.CurrentStep]))
			{
				return EStateTreeRunStatus::Failed;
			}
		}
		LogTick(AI, Data.LastTickLogTime, FString::Printf(
			TEXT("[Aggressive] TICK Running phase=%d step=%d/%d busy=1"),
			Data.ComboPhase, Data.CurrentStep, Data.CurrentChain.Num()));
		return EStateTreeRunStatus::Running;
	}

	if (Combat->WasLastActionInterrupted())
	{
		LogEnter(AI, TEXT("[Aggressive] TICK FAIL action interrupted"));
		return EStateTreeRunStatus::Failed;
	}

	// 当前段已播完：按阶段推进。
	if (Data.ComboPhase == EComboPhase_Opener)
	{
		// 起手式播完 → 进入中间招式段；配了中间段池但无符合距离的段 → 立刻 Failed。
		if (!Config.MiddleMoves.IsEmpty())
		{
			if (!AI->PickComboFromSet(Config.MiddleMoves, Data.CurrentChain))
			{
				LogEnter(AI, TEXT("[Aggressive] TICK FAIL no middle move in range"));
				return EStateTreeRunStatus::Failed;
			}
			Data.CurrentStep = 0;
			// 每段攻击开始重新启用旋转至玩家。
			AI->SetRotateToPlayer(true);
			if (!Combat->PlayMove(Data.CurrentChain[0]))
			{
				return EStateTreeRunStatus::Failed;
			}
			Data.ComboPhase = EComboPhase_Middle;
			LogEnter(AI, FString::Printf(TEXT("[Aggressive] Middle chain %d 段"), Data.CurrentChain.Num()));
			return EStateTreeRunStatus::Running;
		}
		// 未配中间段池 → 直接收尾。
		Data.bFinisherPlayed = false;
		Data.ComboPhase = EComboPhase_Finisher;
	}

	if (Data.ComboPhase == EComboPhase_Middle)
	{
		Data.CurrentStep++;
		if (Data.CurrentChain.IsValidIndex(Data.CurrentStep))
		{
			// 每段攻击开始重新启用旋转至玩家。
			AI->SetRotateToPlayer(true);
			if (!Combat->PlayMove(Data.CurrentChain[Data.CurrentStep]))
			{
				return EStateTreeRunStatus::Failed;
			}
			return EStateTreeRunStatus::Running;
		}
		// 中间招式段播完 → 按当前距离选收尾式。
		Data.bFinisherPlayed = false;
		Data.ComboPhase = EComboPhase_Finisher;
	}

	if (Data.ComboPhase == EComboPhase_Finisher)
	{
		if (!Data.bFinisherPlayed)
		{
			// 特殊招式可直接作为收尾式：先掷一次固定概率骰（无保底，未命中不累加）。
			// 命中 → 播特殊招式收尾（播完即结束）；未命中/冷却中/无可用特殊 → 走传统收尾式。
			if (AI->IsSpecialAvailableFromSet(Config.SpecialMoves))
			{
				const float SpecialChance = FMath::Clamp(Config.ComboEndSpecialChance, 0.f, 1.f);
				if (FMath::FRand() <= SpecialChance)
				{
				URHEnemyActionDefinition* Special = AI->PickSpecialFromSet(Config.SpecialMoves);
				// 每段攻击开始重新启用旋转至玩家（特殊收尾同）。
				AI->SetRotateToPlayer(true);
				if (Special && Combat->PlayAction(Special))
					{
						Data.bCurrentIsSpecial = true;
						Data.bFinisherPlayed = true;
						Data.ComboPhase = EComboPhase_Special;
						LogEnter(AI, FString::Printf(TEXT("[Aggressive] Finisher(Special)=%s"), *GetNameSafe(Special)));
						return EStateTreeRunStatus::Running;
					}
				}
				// 未命中：不增加保底，直接落到传统收尾式。
			}

			// 传统收尾式：配了收尾式池但无符合距离的收尾式 → 立刻 Failed。
			if (!Config.Finishers.IsEmpty())
			{
				URHEnemyMoveDefinition* Finisher = AI->PickFinisher(Config.Finishers);
				if (!Finisher)
				{
					LogEnter(AI, TEXT("[Aggressive] TICK FAIL no finisher in range"));
					return EStateTreeRunStatus::Failed;
				}
				// 每段攻击开始重新启用旋转至玩家（传统收尾同）。
				AI->SetRotateToPlayer(true);
				if (!Combat->PlayMove(Finisher))
				{
					return EStateTreeRunStatus::Failed;
				}
				Data.bFinisherPlayed = true;
				LogEnter(AI, FString::Printf(TEXT("[Aggressive] Finisher=%s"), *GetNameSafe(Finisher)));
				return EStateTreeRunStatus::Running;
			}
			// 未配收尾式池：直接收尾。
			Data.bFinisherPlayed = true;
		}

		AI->ResetCounterBar();
		LogEnter(AI, TEXT("[Aggressive] SUCCEED combo complete"));
		return EStateTreeRunStatus::Succeeded;
	}

	// 兜底：无阶段（理论上不会到）。
	AI->ResetCounterBar();
	LogEnter(AI, TEXT("[Aggressive] SUCCEED fallback"));
	return EStateTreeRunStatus::Succeeded;
}

void FRHEnemyAggressiveTask::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	if (URHEnemyCombatComponent* Combat = RHEnemyAI::GetCombatComponent(Data.AIController))
	{
		Combat->StopCurrentMoveMontage();
	}
}

// ---------------------------------------------------------------------------
// Special
// ---------------------------------------------------------------------------
EStateTreeRunStatus FRHEnemySpecialTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	ARHEnemyBase* Pawn = RHEnemyAI::GetEnemyFromController(Data.AIController);
	URHEnemyAIComponent* AI = RHEnemyAI::GetAIComponent(Data.AIController);
	URHEnemyCombatComponent* Combat = RHEnemyAI::GetCombatComponent(Data.AIController);
	if (!Data.AIController || !Pawn || !AI || !Combat)
	{
		LogEnter(AI, TEXT("[Special] ENTER FAIL Controller/Pawn/AI/Combat missing"));
		return EStateTreeRunStatus::Failed;
	}

	// 受击硬直 / 破防 / 受击动画中不重开特殊招式。
	if (ShouldAbortForDeflect(AI, Combat))
	{
		LogEnter(AI, TEXT("[Special] ENTER FAIL staggered/broken/hitreaction"));
		return EStateTreeRunStatus::Failed;
	}

	const FRHPlayerCombatSnapshot& Snap = AI->GetSnapshot();
	if (!Snap.Player || Snap.bPlayerHealing || !AI->IsSpecialAvailable())
	{
		LogEnter(AI, FString::Printf(
			TEXT("[Special] ENTER FAIL player=%d healing=%d specialAvailable=%d"),
			Snap.Player ? 1 : 0, Snap.bPlayerHealing ? 1 : 0,
			AI->IsSpecialAvailable() ? 1 : 0));
		return EStateTreeRunStatus::Failed;
	}

	URHEnemyActionDefinition* Action = AI->PickSpecial();
	if (!Action)
	{
		LogEnter(AI, TEXT("[Special] ENTER FAIL PickSpecial returned null"));
		return EStateTreeRunStatus::Failed;
	}
	AI->SetRotateToPlayer(true);

	if (!Combat->PlayAction(Action))
	{
		LogEnter(AI, FString::Printf(
			TEXT("[Special] ENTER FAIL PlayAction %s"),
			*GetNameSafe(Action)));
		return EStateTreeRunStatus::Failed;
	}
	LogEnter(AI, FString::Printf(
		TEXT("[Special] ENTER Running action=%s"),
		*GetNameSafe(Action)));
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FRHEnemySpecialTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	URHEnemyAIComponent* AI = RHEnemyAI::GetAIComponent(Data.AIController);
	URHEnemyCombatComponent* Combat = RHEnemyAI::GetCombatComponent(Data.AIController);
	if (!Data.AIController || !AI || !Combat)
	{
		LogEnter(AI, TEXT("[Special] TICK FAIL Controller/AI/Combat missing"));
		return EStateTreeRunStatus::Failed;
	}

	if (ShouldAbortForDeflect(AI, Combat))
	{
		LogEnter(AI, TEXT("[Special] TICK FAIL staggered/broken/hitreaction"));
		return EStateTreeRunStatus::Failed;
	}

	if (AI->GetSnapshot().bPlayerHealing)
	{
		LogEnter(AI, TEXT("[Special] TICK FAIL player healing, stopping action"));
		Combat->StopCurrentMoveMontage();
		return EStateTreeRunStatus::Failed;
	}

	if (!Combat->IsBusy())
	{
		if (Data.bResetCounterBarOnComplete)
		{
			AI->ResetCounterBar();
		}
		LogEnter(AI, TEXT("[Special] SUCCEED action finished"));
		return EStateTreeRunStatus::Succeeded;
	}
	return EStateTreeRunStatus::Running;
}

void FRHEnemySpecialTask::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	if (URHEnemyCombatComponent* Combat = RHEnemyAI::GetCombatComponent(Data.AIController))
	{
		Combat->StopCurrentMoveMontage();
	}
}

// ---------------------------------------------------------------------------
// Guard
// ---------------------------------------------------------------------------
EStateTreeRunStatus FRHEnemyGuardTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	Data.Elapsed = 0.f;
	Data.Wander.MoveMode = 0;
	Data.Wander.MoveTimer = 0.f;

	ARHEnemyBase* Pawn = RHEnemyAI::GetEnemyFromController(Data.AIController);
	URHEnemyAIComponent* AI = RHEnemyAI::GetAIComponent(Data.AIController);
	URHEnemyCombatComponent* Combat = RHEnemyAI::GetCombatComponent(Data.AIController);
	if (!Data.AIController || !Pawn || !AI || !Combat)
	{
		LogEnter(AI, TEXT("[Guard] ENTER FAIL Controller/AI/Combat missing"));
		return EStateTreeRunStatus::Failed;
	}

	if (ShouldAbortForDeflect(AI, Combat))
	{
		LogEnter(AI, TEXT("[Guard] ENTER FAIL staggered/broken/hitreaction"));
		return EStateTreeRunStatus::Failed;
	}

	// 进入守卫立刻播蒙太奇并切防御态；未配防御蒙太奇直接失败。
	UAnimMontage* GuardMontage = AI->GetCurrentConfig().GuardMontage.LoadSynchronous();
	if (!GuardMontage)
	{
		LogEnter(AI, TEXT("[Guard] ENTER FAIL GuardMontage missing"));
		return EStateTreeRunStatus::Failed;
	}
	Combat->SetGuarding(true);
	Combat->PlayMontage(GuardMontage);
	AI->SetRotateToPlayer(true);
	// 防御固定 200 速度自由走动，不依赖 Feel 配置。
	SetEnemyWalkSpeed(Pawn, 200.f);
	LogEnter(AI, TEXT("[Guard] ENTER Running"));
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FRHEnemyGuardTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	ARHEnemyBase* Pawn = RHEnemyAI::GetEnemyFromController(Data.AIController);
	URHEnemyAIComponent* AI = RHEnemyAI::GetAIComponent(Data.AIController);
	if (!Data.AIController || !Pawn || !AI)
	{
		LogEnter(AI, TEXT("[Guard] TICK FAIL Controller/Pawn/AI missing"));
		return EStateTreeRunStatus::Failed;
	}

	if (ShouldAbortForDeflect(AI, RHEnemyAI::GetCombatComponent(Data.AIController)))
	{
		LogEnter(AI, TEXT("[Guard] TICK FAIL staggered/broken/hitreaction"));
		return EStateTreeRunStatus::Failed;
	}

	Data.Elapsed += DeltaTime;
	// 守卫态也可以自由走动（逻辑同 Idle）。
	TickWander(Pawn, AI, Data.Wander, DeltaTime);
	// TickWander 会按 Feel.IdleStrafeSpeed 设置速度，防御时强制盖回 200。
	SetEnemyWalkSpeed(Pawn, 200.f);
	if (Data.bEndWhenPlayerStopsAttacking)
	{
		if (AI->GetSnapshot().bPlayerAttacking)
		{
			Data.bWaitingForPlayerStop = false;
			Data.StopGraceElapsed = 0.f;
		}
		else if (!Data.bWaitingForPlayerStop)
		{
			Data.bWaitingForPlayerStop = true;
			Data.StopGraceElapsed = 0.f;
		}
		else
		{
			Data.StopGraceElapsed += DeltaTime;
			if (Data.StopGraceElapsed >= Data.PlayerStopGraceSeconds)
			{
				LogEnter(AI, TEXT("[Guard] SUCCEED player stopped attacking"));
				return EStateTreeRunStatus::Succeeded;
			}
		}
	}
	if (Data.MaxGuardDuration > 0.f && Data.Elapsed >= Data.MaxGuardDuration)
	{
		LogEnter(AI, TEXT("[Guard] SUCCEED max duration reached"));
		return EStateTreeRunStatus::Succeeded;
	}
	return EStateTreeRunStatus::Running;
}

void FRHEnemyGuardTask::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	if (URHEnemyCombatComponent* Combat = RHEnemyAI::GetCombatComponent(Data.AIController))
	{
		Combat->SetGuarding(false);
		Combat->StopCurrentMoveMontage();
	}
}

// ---------------------------------------------------------------------------
// Deflect
// ---------------------------------------------------------------------------
EStateTreeRunStatus FRHEnemyDeflectTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	URHEnemyAIComponent* AI = RHEnemyAI::GetAIComponent(Data.AIController);
	URHEnemyCombatComponent* Combat = RHEnemyAI::GetCombatComponent(Data.AIController);
	if (!Data.AIController || !AI || !Combat)
	{
		LogEnter(AI, TEXT("[Deflect] ENTER FAIL Controller/AI/Combat missing"));
		return EStateTreeRunStatus::Failed;
	}

	if (ShouldAbortForStaggerExcludingDeflect(AI, Combat))
	{
		LogEnter(AI, TEXT("[Deflect] ENTER FAIL staggered/broken/hitreaction"));
		return EStateTreeRunStatus::Failed;
	}

	// 弹开已由反击条打空时直接触发（TriggerDeflect 播蒙太奇+通知玩家）。
	// StateTree 这里只是保持 Running，等待同一段蒙太奇播完后重置反击条，避免重复触发。
	UAnimMontage* DeflectMontage = AI->GetCurrentConfig().DeflectMontage.LoadSynchronous();
	if (!DeflectMontage)
	{
		LogEnter(AI, TEXT("[Deflect] ENTER FAIL DeflectMontage missing"));
		return EStateTreeRunStatus::Failed;
	}
	LogEnter(AI, TEXT("[Deflect] ENTER Running"));
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FRHEnemyDeflectTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	URHEnemyAIComponent* AI = RHEnemyAI::GetAIComponent(Data.AIController);
	URHEnemyCombatComponent* Combat = RHEnemyAI::GetCombatComponent(Data.AIController);
	if (!Data.AIController || !AI || !Combat)
	{
		LogEnter(AI, TEXT("[Deflect] TICK FAIL Controller/AI/Combat missing"));
		return EStateTreeRunStatus::Failed;
	}

	if (ShouldAbortForStaggerExcludingDeflect(AI, Combat))
	{
		LogEnter(AI, TEXT("[Deflect] TICK FAIL staggered/broken/hitreaction"));
		return EStateTreeRunStatus::Failed;
	}

	if (!Combat->IsMontagePlaying())
	{
		if (Data.bResetCounterBarOnSuccess)
		{
			AI->ResetCounterBar();
		}
		LogEnter(AI, TEXT("[Deflect] SUCCEED montage finished"));
		return EStateTreeRunStatus::Succeeded;
	}
	return EStateTreeRunStatus::Running;
}

void FRHEnemyDeflectTask::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	URHEnemyAIComponent* AI = RHEnemyAI::GetAIComponent(Data.AIController);
	URHEnemyCombatComponent* Combat = RHEnemyAI::GetCombatComponent(Data.AIController);
	if (AI)
	{
		AI->CloseDeflectWindow();
		AI->ClearDeflectSucceeded();
	}
	if (Combat)
	{
		Combat->StopCurrentMoveMontage();
	}
}

// ---------------------------------------------------------------------------
// Dodge
// ---------------------------------------------------------------------------
EStateTreeRunStatus FRHEnemyDodgeTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	Data.Elapsed = 0.f;

	ARHEnemyBase* Pawn = RHEnemyAI::GetEnemyFromController(Data.AIController);
	URHEnemyAIComponent* AI = RHEnemyAI::GetAIComponent(Data.AIController);
	URHEnemyCombatComponent* Combat = RHEnemyAI::GetCombatComponent(Data.AIController);
	if (!Data.AIController || !Pawn || !AI || !Combat)
	{
		LogEnter(AI, TEXT("[Dodge] ENTER FAIL Controller/Pawn/AI/Combat missing"));
		return EStateTreeRunStatus::Failed;
	}

	if (ShouldAbortForDeflect(AI, Combat))
	{
		LogEnter(AI, TEXT("[Dodge] ENTER FAIL staggered/broken/hitreaction"));
		return EStateTreeRunStatus::Failed;
	}

	if (!AI->CanDodge())
	{
		LogEnter(AI, TEXT("[Dodge] ENTER FAIL cooldown"));
		return EStateTreeRunStatus::Failed;
	}
	AI->NotifyDodgeStarted();

	// 闪避无敌帧：整个闪避期间免伤、免受击（和玩家闪避一致）。
	TArray<float> Weights = { Data.FrontWeight, Data.BackWeight, Data.LeftWeight, Data.RightWeight };
	const float TotalWeight = FMath::Max(0.f, Weights[0]) + FMath::Max(0.f, Weights[1])
		+ FMath::Max(0.f, Weights[2]) + FMath::Max(0.f, Weights[3]);
	float Roll = TotalWeight > 0.f ? FMath::FRandRange(0.f, TotalWeight) : 0.f;

	ERHEnemyDodgeDirection Direction = ERHEnemyDodgeDirection::Back;
	if (TotalWeight <= 0.f)
	{
		Direction = ERHEnemyDodgeDirection::Back;
	}
	else
	{
		for (int32 Index = 0; Index < 4; ++Index)
		{
			Roll -= FMath::Max(0.f, Weights[Index]);
			if (Roll <= 0.f)
			{
				Direction = static_cast<ERHEnemyDodgeDirection>(Index);
				break;
			}
		}
	}

	FName Section = TEXT("B");
	FVector Dir = -Pawn->GetActorForwardVector();
	switch (Direction)
	{
	case ERHEnemyDodgeDirection::Front:
		Section = TEXT("F");
		Dir = Pawn->GetActorForwardVector();
		break;
	case ERHEnemyDodgeDirection::Back:
		Section = TEXT("B");
		Dir = -Pawn->GetActorForwardVector();
		break;
	case ERHEnemyDodgeDirection::Left:
		Section = TEXT("L");
		Dir = -Pawn->GetActorRightVector();
		break;
	case ERHEnemyDodgeDirection::Right:
		Section = TEXT("R");
		Dir = Pawn->GetActorRightVector();
		break;
	}

	UAnimMontage* DodgeMontage = AI->GetCurrentConfig().DodgeMontage.LoadSynchronous();
	Data.bUsingMontage = (DodgeMontage != nullptr);
	if (DodgeMontage)
	{
		// 蒙太奇带 F/L/R/B section（L=左侧、R=右侧）；位移由根运动决定。
		Combat->PlayMontageSection(DodgeMontage, Section);
	}
	else
	{
		// 未配蒙太奇时的兜底位移（DodgeDistance 仅此时生效）。
		Pawn->LaunchCharacter(Dir * FMath::Max(Data.DodgeDistance, 100.f), false, false);
	}
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FRHEnemyDodgeTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	URHEnemyAIComponent* AI = RHEnemyAI::GetAIComponent(Data.AIController);
	URHEnemyCombatComponent* Combat = RHEnemyAI::GetCombatComponent(Data.AIController);
	if (!Data.AIController || !AI || !Combat)
	{
		LogEnter(AI, TEXT("[Dodge] TICK FAIL Controller/AI/Combat missing"));
		return EStateTreeRunStatus::Failed;
	}

	if (ShouldAbortForDeflect(AI, Combat))
	{
		LogEnter(AI, TEXT("[Dodge] TICK FAIL staggered/broken/hitreaction"));
		return EStateTreeRunStatus::Failed;
	}

	Data.Elapsed += DeltaTime;
	if (Data.bUsingMontage)
	{
		if (!Combat->IsMontagePlaying())
		{
			LogEnter(AI, TEXT("[Dodge] SUCCEED montage finished"));
			return EStateTreeRunStatus::Succeeded;
		}
		return EStateTreeRunStatus::Running;
	}
	if (Data.Elapsed > 0.15f)
	{
		LogEnter(AI, TEXT("[Dodge] SUCCEED fallback move finished"));
		return EStateTreeRunStatus::Succeeded;
	}
	return EStateTreeRunStatus::Running;
}

void FRHEnemyDodgeTask::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	if (URHEnemyCombatComponent* Combat = RHEnemyAI::GetCombatComponent(Data.AIController))
	{
		Combat->StopCurrentMoveMontage();
	}
}

// ---------------------------------------------------------------------------
// Down
// ---------------------------------------------------------------------------
EStateTreeRunStatus FRHEnemyDownTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	ARHEnemyBase* Pawn = RHEnemyAI::GetEnemyFromController(Data.AIController);
	URHEnemyAIComponent* AI = RHEnemyAI::GetAIComponent(Data.AIController);
	URHEnemyCombatComponent* Combat = RHEnemyAI::GetCombatComponent(Data.AIController);
	if (!Data.AIController || !AI || !Combat)
	{
		LogEnter(AI, TEXT("[Down] ENTER FAIL Controller/AI/Combat missing"));
		return EStateTreeRunStatus::Failed;
	}

	// 破防进入该状态；不在破防就不该在这里。
	if (!AI->IsBroken() && !AI->IsDowned())
	{
		LogEnter(AI, TEXT("[Down] ENTER SUCCEED not broken/downed, skipping"));
		return EStateTreeRunStatus::Succeeded;
	}

	LogEnter(AI, FString::Printf(
		TEXT("[Down] ENTER Running broken=%d downed=%d"),
		AI->IsBroken() ? 1 : 0, AI->IsDowned() ? 1 : 0));
	if (AI)
	{
		AI->SetRotateToPlayer(false);
	}
	if (Pawn)
	{
		Pawn->SetExecutionRangeActive(true);
	}
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FRHEnemyDownTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	ARHEnemyBase* Pawn = RHEnemyAI::GetEnemyFromController(Data.AIController);
	URHEnemyAIComponent* AI = RHEnemyAI::GetAIComponent(Data.AIController);
	URHEnemyCombatComponent* Combat = RHEnemyAI::GetCombatComponent(Data.AIController);
	if (!Data.AIController || !AI || !Combat)
	{
		LogEnter(AI, TEXT("[Down] TICK FAIL Controller/AI/Combat missing"));
		return EStateTreeRunStatus::Failed;
	}

	if (AI->IsBroken())
	{
		// 破防中：播倒地 start 段（蒙太奇内部自动进 loop 循环）；
		// 被玩家命中会走受击动画打断，受击播完（当前无蒙太奇）就重新播 start；
		// 共振归零时 AI 组件播 end 段并解除破防。
		if (!Combat->IsMontagePlaying())
		{
			Combat->PlayMontageSection(AI->GetCurrentConfig().DownMontage.LoadSynchronous(), TEXT("start"));
		}
		if (Pawn)
		{
			Pawn->SetExecutionRangeActive(true);
		}
		return EStateTreeRunStatus::Running;
	}

	// 破防已解除（end 段由 AI 组件负责播放）。
	LogEnter(AI, TEXT("[Down] SUCCEED break resolved"));
	return EStateTreeRunStatus::Succeeded;
}

void FRHEnemyDownTask::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	if (ARHEnemyBase* Pawn = RHEnemyAI::GetEnemyFromController(Data.AIController))
	{
		if (URHEnemyAIComponent* AI = RHEnemyAI::GetAIComponent(Data.AIController))
		{
			AI->SetRotateToPlayer(true);
		}
		Pawn->SetExecutionRangeActive(false);
	}
	// 不主动停蒙太奇：让 end（起身）动画自然播完；受击打断由受击系统处理。
}

// ---------------------------------------------------------------------------
// Execution
// ---------------------------------------------------------------------------
EStateTreeRunStatus FRHEnemyExecutionTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	ARHEnemyBase* Pawn = RHEnemyAI::GetEnemyFromController(Data.AIController);
	URHEnemyAIComponent* AI = RHEnemyAI::GetAIComponent(Data.AIController);
	URHEnemyCombatComponent* Combat = RHEnemyAI::GetCombatComponent(Data.AIController);
	if (!Data.AIController || !Pawn || !AI || !Combat)
	{
		LogEnter(AI, TEXT("[Execution] ENTER FAIL Controller/Pawn/AI/Combat missing"));
		return EStateTreeRunStatus::Failed;
	}

	if (!AI->IsBroken())
	{
		LogEnter(AI, TEXT("[Execution] ENTER SUCCEED not broken"));
		return EStateTreeRunStatus::Succeeded;
	}

	AActor* Player = AI->GetPlayerTarget();
	if (!Player)
	{
		LogEnter(AI, TEXT("[Execution] ENTER FAIL player missing"));
		return EStateTreeRunStatus::Failed;
	}

	AI->BeginExecution();

	URHWeaponDefinition* WeaponDef = nullptr;
	if (UKnsCombatComponent* PlayerCombat = Player->FindComponentByClass<UKnsCombatComponent>())
	{
		WeaponDef = PlayerCombat->WeaponDefinition;
	}
	const float ForwardDistance = WeaponDef ? WeaponDef->ExecutedDistance : 120.f;

	// 把玩家放到敌人正前方，双方都朝对方，保证两边蒙太奇对齐。
	const FVector EnemyLocation = Pawn->GetActorLocation();
	const FVector EnemyForward = Pawn->GetActorForwardVector().GetSafeNormal2D();
	const FVector TargetPlayerLocation = EnemyLocation + EnemyForward * ForwardDistance;
	Player->SetActorLocation(TargetPlayerLocation, false, nullptr, ETeleportType::TeleportPhysics);

	const FVector ToEnemy = (EnemyLocation - Player->GetActorLocation()).GetSafeNormal2D();
	if (!ToEnemy.IsNearlyZero())
	{
		Player->SetActorRotation(ToEnemy.Rotation());
	}

	const FVector ToPlayer = (Player->GetActorLocation() - EnemyLocation).GetSafeNormal2D();
	if (!ToPlayer.IsNearlyZero())
	{
		Pawn->SetActorRotation(ToPlayer.Rotation());
	}

	if (IRHCombatActionInterface* PlayerInterface = Cast<IRHCombatActionInterface>(Player))
	{
		PlayerInterface->StartExecution(Pawn);
	}

	UAnimMontage* EnemyExecMontage = WeaponDef ? WeaponDef->ExecutedMontage.Get() : nullptr;
	if (!EnemyExecMontage)
	{
		LogEnter(AI, TEXT("[Execution] ENTER FAIL Enemy ExecutedMontage missing in weapon def"));
		return EStateTreeRunStatus::Failed;
	}
	Combat->PlayMontage(EnemyExecMontage);
	LogEnter(AI, FString::Printf(TEXT("[Execution] ENTER Running forward=%.1f"), ForwardDistance));
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FRHEnemyExecutionTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	URHEnemyAIComponent* AI = RHEnemyAI::GetAIComponent(Data.AIController);
	URHEnemyCombatComponent* Combat = RHEnemyAI::GetCombatComponent(Data.AIController);
	if (!Data.AIController || !AI || !Combat)
	{
		LogEnter(AI, TEXT("[Execution] TICK FAIL Controller/AI/Combat missing"));
		return EStateTreeRunStatus::Failed;
	}

	if (Combat->IsMontagePlaying())
	{
		return EStateTreeRunStatus::Running;
	}

	AI->FinishExecution();
	LogEnter(AI, TEXT("[Execution] SUCCEED execution montage finished"));
	return EStateTreeRunStatus::Succeeded;
}

void FRHEnemyExecutionTask::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	if (URHEnemyCombatComponent* Combat = RHEnemyAI::GetCombatComponent(Data.AIController))
	{
		Combat->StopCurrentMoveMontage();
	}
}

// ---------------------------------------------------------------------------
// Phase Transition
// ---------------------------------------------------------------------------
EStateTreeRunStatus FRHEnemyPhaseTransitionTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	URHEnemyAIComponent* AI = RHEnemyAI::GetAIComponent(Data.AIController);
	URHEnemyCombatComponent* Combat = RHEnemyAI::GetCombatComponent(Data.AIController);
	if (!Data.AIController || !AI || !Combat)
	{
		LogEnter(AI, TEXT("[PhaseTransition] ENTER FAIL Controller/AI/Combat missing"));
		return EStateTreeRunStatus::Failed;
	}

	if (ShouldAbortForStagger(AI, Combat))
	{
		LogEnter(AI, TEXT("[PhaseTransition] ENTER FAIL staggered/broken/hitreaction"));
		return EStateTreeRunStatus::Failed;
	}

	AI->SetRotateToPlayer(false);
	// 阶段号按 Phases 数组顺序自动推进（Phases[i] = 第 i+2 阶段，开局默认 phase 1），
	// 不需要在树资产里手动配置 TargetPhaseIndex。
	const int32 NextPhase = AI->GetCurrentPhase() + 1;
	AI->SetPhase(NextPhase);

	// EntranceMontage 可选：配了才播（Running 等播完），没配直接成功——
	// 否则转阶段任务 Failed 回根会立刻被同一条血量条件重新触发，陷入死循环（处决后树卡死）。
	UAnimMontage* EntranceMontage = AI->GetCurrentConfig().EntranceMontage.LoadSynchronous();
	if (EntranceMontage)
	{
		Combat->PlayMontage(EntranceMontage);
		LogEnter(AI, FString::Printf(TEXT("[PhaseTransition] ENTER Running phase=%d (montage)"), NextPhase));
		return EStateTreeRunStatus::Running;
	}
	LogEnter(AI, FString::Printf(TEXT("[PhaseTransition] SUCCEED phase=%d (no montage)"), NextPhase));
	return EStateTreeRunStatus::Succeeded;
}

EStateTreeRunStatus FRHEnemyPhaseTransitionTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	URHEnemyAIComponent* AI = RHEnemyAI::GetAIComponent(Data.AIController);
	URHEnemyCombatComponent* Combat = RHEnemyAI::GetCombatComponent(Data.AIController);
	if (!Data.AIController || !AI || !Combat)
	{
		LogEnter(AI, TEXT("[PhaseTransition] TICK FAIL Controller/AI/Combat missing"));
		return EStateTreeRunStatus::Failed;
	}
	if (ShouldAbortForStagger(AI, Combat))
	{
		LogEnter(AI, TEXT("[PhaseTransition] TICK FAIL staggered/broken/hitreaction"));
		return EStateTreeRunStatus::Failed;
	}
	if (Combat->IsMontagePlaying())
	{
		return EStateTreeRunStatus::Running;
	}
	LogEnter(AI, TEXT("[PhaseTransition] SUCCEED entrance montage finished"));
	return EStateTreeRunStatus::Succeeded;
}

void FRHEnemyPhaseTransitionTask::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	if (URHEnemyAIComponent* AI = RHEnemyAI::GetAIComponent(Data.AIController))
	{
		AI->SetRotateToPlayer(true);
	}
	if (URHEnemyCombatComponent* Combat = RHEnemyAI::GetCombatComponent(Data.AIController))
	{
		Combat->StopCurrentMoveMontage();
	}
}

// ---------------------------------------------------------------------------
// Wait Recover
// ---------------------------------------------------------------------------
EStateTreeRunStatus FRHEnemyWaitRecoverTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	if (URHEnemyAIComponent* AI = RHEnemyAI::GetAIComponent(Data.AIController))
	{
		AI->SetRotateToPlayer(false);
	}
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FRHEnemyWaitRecoverTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	URHEnemyAIComponent* AI = RHEnemyAI::GetAIComponent(Data.AIController);
	if (!Data.AIController || !AI)
	{
		LogEnter(AI, TEXT("[WaitRecover] TICK FAIL Controller/AI missing"));
		return EStateTreeRunStatus::Failed;
	}
	URHEnemyCombatComponent* Combat = RHEnemyAI::GetCombatComponent(Data.AIController);
	// 蒙太奇真正播完（含 blend out/替换间隙）才允许重选：连续命中时旧受击蒙太奇被替换/Stop
	// 的瞬间 CurrentReactionMontage 会短暂为 null，用 IsReactionAnimationActive 兜底防止提前恢复反击。
	if (!AI->IsStaggered() && !(Combat && Combat->IsReactionAnimationActive()))
	{
		AI->SetRotateToPlayer(true);
		LogEnter(AI, TEXT("[WaitRecover] SUCCEED stagger/hitreaction ended"));
		return EStateTreeRunStatus::Succeeded;
	}
	LogTick(AI, Data.LastTickLogTime, FString::Printf(
		TEXT("[WaitRecover] TICK Running staggered=%d hitReaction=%d"),
		AI->IsStaggered() ? 1 : 0,
		(Combat && Combat->IsHitReactionPlaying()) ? 1 : 0));
	return EStateTreeRunStatus::Running;
}

void FRHEnemyWaitRecoverTask::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	if (URHEnemyAIComponent* AI = RHEnemyAI::GetAIComponent(Data.AIController))
	{
		// EnterState 关闭了自动转向：任何退出路径（含被打断/事件转移）都恢复，
		// 否则受击/弹反恢复后敌人不面向玩家（Aggressive 等任务不重开旋转）。
		AI->SetRotateToPlayer(true);
	}
}

// ---------------------------------------------------------------------------
// Idle
// ---------------------------------------------------------------------------
EStateTreeRunStatus FRHEnemyIdleTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	ARHEnemyBase* Pawn = RHEnemyAI::GetEnemyFromController(Data.AIController);
	URHEnemyAIComponent* AI = RHEnemyAI::GetAIComponent(Data.AIController);
	if (!Data.AIController || !Pawn || !AI)
	{
		LogEnter(AI, TEXT("[Idle] ENTER FAIL Controller/Pawn/AI missing"));
		return EStateTreeRunStatus::Failed;
	}
	if (ShouldAbortForStagger(AI, RHEnemyAI::GetCombatComponent(Data.AIController)))
	{
		LogEnter(AI, TEXT("[Idle] ENTER FAIL staggered/broken/hitreaction"));
		return EStateTreeRunStatus::Failed;
	}

	Data.Elapsed = 0.f;
	Data.Wander.MoveMode = 0;
	Data.Wander.MoveTimer = 0.f; // 首帧立刻选动作，禁止站桩
	AI->SetRotateToPlayer(true);

	SetEnemyWalkSpeed(Pawn, AI->GetCurrentConfig().Attributes.WalkSpeed);
	LogEnter(AI, FString::Printf(
		TEXT("[Idle] ENTER Running duration=%.1f face=%d"),
		Data.Duration, Data.bFacePlayer ? 1 : 0));
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FRHEnemyIdleTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	ARHEnemyBase* Pawn = RHEnemyAI::GetEnemyFromController(Data.AIController);
	URHEnemyAIComponent* AI = RHEnemyAI::GetAIComponent(Data.AIController);
	if (!Data.AIController || !Pawn || !AI)
	{
		LogEnter(AI, TEXT("[Idle] TICK FAIL Controller/Pawn/AI missing"));
		return EStateTreeRunStatus::Failed;
	}

	// 受击硬直 / 破防中不面向玩家、不走位（倒地/硬直时保持姿势）。
	if (ShouldAbortForStagger(AI, RHEnemyAI::GetCombatComponent(Data.AIController)))
	{
		LogEnter(AI, TEXT("[Idle] TICK FAIL staggered/broken/hitreaction"));
		return EStateTreeRunStatus::Failed;
	}

	Data.Elapsed += DeltaTime;

	// 自由走动：左/右/后退循环，永不站桩。
	TickWander(Pawn, AI, Data.Wander, DeltaTime);

	if (Data.Duration > 0.f && Data.Elapsed >= Data.Duration)
	{
		LogEnter(AI, FString::Printf(
			TEXT("[Idle] SUCCEED elapsed=%.1f duration=%.1f"),
			Data.Elapsed, Data.Duration));
		return EStateTreeRunStatus::Succeeded;
	}
	LogTick(AI, Data.LastTickLogTime, FString::Printf(
		TEXT("[Idle] TICK Running elapsed=%.1f duration=%.1f dist2D=%.1f"),
		Data.Elapsed, Data.Duration, AI->GetSnapshot().Distance2D));
	return EStateTreeRunStatus::Running;
}

void FRHEnemyIdleTask::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	if (ARHEnemyBase* Pawn = RHEnemyAI::GetEnemyFromController(Data.AIController))
	{
		if (URHEnemyAIComponent* AI = RHEnemyAI::GetAIComponent(Data.AIController))
		{
			SetEnemyWalkSpeed(Pawn, AI->GetCurrentConfig().Attributes.WalkSpeed);
			AI->DebugPrint(TEXT("[Idle] EXIT"));
		}
	}
}

// ---------------------------------------------------------------------------
// Death
// ---------------------------------------------------------------------------
EStateTreeRunStatus FRHEnemyDeathTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	ARHEnemyBase* Pawn = RHEnemyAI::GetEnemyFromController(Data.AIController);
	URHEnemyAIComponent* AI = RHEnemyAI::GetAIComponent(Data.AIController);
	URHEnemyCombatComponent* Combat = RHEnemyAI::GetCombatComponent(Data.AIController);
	if (!Data.AIController || !Pawn || !AI || !Combat)
	{
		LogEnter(AI, TEXT("[Death] ENTER FAIL Controller/Pawn/AI/Combat missing"));
		return EStateTreeRunStatus::Failed;
	}

	Data.Elapsed = 0.f;
	Data.bDeathMontageStarted = false;

	AI->SetRotateToPlayer(false);
	Data.AIController->StopMovement();
	Combat->StopCurrentMoveMontage();
	Combat->DestroySpawnedWeapon(); // 死亡时把武器一起清除
	// 死亡瞬间：关闭敌人身上全部 UI（锁定白点/浮层血条）+ 破防同款时间变慢。
	Pawn->HideAllUI();
	Combat->TriggerBreakTimeDilation();
	UAnimMontage* DeathMontage = AI->GetCurrentConfig().DeathMontage.LoadSynchronous();
	Data.bDeathMontageStarted = DeathMontage != nullptr && Combat->PlayDeathMontage(DeathMontage);

	// 死亡开始立刻通知玩家：解除锁定（OnLockReleased 会顺带清敌人信息 UI）。
	if (AActor* Player = AI->GetPlayerTarget())
	{
		if (IRHCombatActionInterface* PlayerInterface = Cast<IRHCombatActionInterface>(Player))
		{
			PlayerInterface->NotifyEnemyDefeated(Pawn);
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("RHEnemy: %s died"), *GetNameSafe(Pawn));
	LogEnter(AI, TEXT("[Death] ENTER Running"));
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FRHEnemyDeathTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	ARHEnemyBase* Pawn = RHEnemyAI::GetEnemyFromController(Data.AIController);
	URHEnemyAIComponent* AI = RHEnemyAI::GetAIComponent(Data.AIController);
	URHEnemyCombatComponent* Combat = RHEnemyAI::GetCombatComponent(Data.AIController);
	if (!Data.AIController || !Pawn || !AI || !Combat)
	{
		LogEnter(AI, TEXT("[Death] TICK FAIL Controller/Pawn/AI/Combat missing"));
		return EStateTreeRunStatus::Failed;
	}

	Data.Elapsed += DeltaTime;
	const bool bHasDeathMontage = !AI->GetCurrentConfig().DeathMontage.IsNull();
	const bool bDone = bHasDeathMontage
		? (Data.bDeathMontageStarted && !Combat->IsMontagePlaying() && Data.Elapsed > 0.3f)
		: (Data.Elapsed > 2.f);
	if (bDone)
	{
		Data.AIController->StopMovement();
		Pawn->Destroy();
		LogEnter(AI, TEXT("[Death] SUCCEED death montage done, destroying self"));
		return EStateTreeRunStatus::Succeeded;
	}
	return EStateTreeRunStatus::Running;
}

// ---------------------------------------------------------------------------
// Debug Log
// ---------------------------------------------------------------------------
EStateTreeRunStatus FRHEnemyDebugLogTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	UE_LOG(LogTemp, Warning, TEXT("[EnemyAI] %s"), *Data.Message);
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FRHEnemyDebugLogTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	return EStateTreeRunStatus::Running;
}
