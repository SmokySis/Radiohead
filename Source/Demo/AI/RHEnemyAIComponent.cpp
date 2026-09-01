#include "RHEnemyAIComponent.h"

#include "AbilitySystemInterface.h"
#include "Animation/AnimMontage.h"
#include "AIController.h"
#include "Components/StateTreeAIComponent.h"
#include "Demo/AI/RHEnemyActionDefinition.h"
#include "Demo/AI/RHEnemyCombatComponent.h"
#include "Demo/AI/RHEnemyDefinition.h"
#include "Demo/Character/RHEnemyBase.h"
#include "Demo/Combat/KnsCombatComponent.h"
#include "Demo/Combat/RHCombatActionInterface.h"
#include "Demo/Combat/RHHitData.h"
#include "Demo/GAS/KnsAbilitySystemComponent.h"
#include "Demo/GAS/KnsCommonAttributeSet.h"
#include "Demo/Onom/RHWeaponDefinition.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

URHEnemyAIComponent::URHEnemyAIComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void URHEnemyAIComponent::BeginPlay()
{
	Super::BeginPlay();
}

void URHEnemyAIComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bInitialized)
	{
		Initialize();
	}

	RefreshPlayerReference();
	RefreshSnapshot();
	TickRotateToPlayer(DeltaTime);
	TickCounterBar(DeltaTime);
	TickCounterAttack(DeltaTime);
	TickResonance(DeltaTime);
	TickBreakState(DeltaTime);
	MaybeRequestExecution();
	SpecialCooldownRemaining = FMath::Max(0.f, SpecialCooldownRemaining - DeltaTime);
	RemoteCooldownRemaining = FMath::Max(0.f, RemoteCooldownRemaining - DeltaTime);

	if (bShowDebugPrints)
	{
		if (UWorld* World = GetWorld())
		{
			const double Now = World->GetTimeSeconds();
			if (LastDebugLogTime < 0.0 || Now - LastDebugLogTime >= 1.0)
			{
				LastDebugLogTime = Now;
				APawn* AsPawn = Cast<APawn>(GetOwner());
				AAIController* Controller = AsPawn ? Cast<AAIController>(AsPawn->GetController()) : nullptr;
				DebugPrint(FString::Printf(
					TEXT("Tick Owner=%s Controller=%s Player=%s Dist=%.1f Dist2D=%.1f Intent=%s Busy=%d Attacking=%d Broken=%d Downed=%d Phase=%d"),
					*GetNameSafe(GetOwner()),
					*GetNameSafe(Controller),
					*GetNameSafe(PlayerTarget),
					Snapshot.Distance,
					Snapshot.Distance2D,
					*Snapshot.PlayerIntent.ToString(),
					Snapshot.bPlayerBusy ? 1 : 0,
					Snapshot.bPlayerAttacking ? 1 : 0,
					bResonanceBroken ? 1 : 0,
					bIsDowned ? 1 : 0,
					CurrentPhaseIndex));
			}
		}
	}
}

void URHEnemyAIComponent::Initialize()
{
	if (bInitialized)
	{
		return;
	}
	bInitialized = true;

	TagPlayerBusy = FGameplayTag::RequestGameplayTag(TEXT("State.Player.Busy"), false);
	TagPlayerAttacking = FGameplayTag::RequestGameplayTag(TEXT("State.Player.Attacking"), false);
	TagPlayerSkill = FGameplayTag::RequestGameplayTag(TEXT("State.Player.Skill"), false);
	TagPlayerLoad = FGameplayTag::RequestGameplayTag(TEXT("State.Player.Load"), false);
	TagPlayerToss = FGameplayTag::RequestGameplayTag(TEXT("State.Player.Toss"), false);
	TagPlayerDodge = FGameplayTag::RequestGameplayTag(TEXT("State.Player.Dodge"), false);
	TagPlayerBlock = FGameplayTag::RequestGameplayTag(TEXT("State.Player.Block"), false);
	TagPlayerParry = FGameplayTag::RequestGameplayTag(TEXT("State.Player.Parry"), false);
	TagPlayerGuarding = FGameplayTag::RequestGameplayTag(TEXT("Status.Guarding"), false);
	TagPlayerHealing = FGameplayTag::RequestGameplayTag(TEXT("AI.PlayerIntent.Healing"), false);
	TagIntentMelee = FGameplayTag::RequestGameplayTag(TEXT("AI.PlayerIntent.Melee"), false);
	TagIntentRanged = FGameplayTag::RequestGameplayTag(TEXT("AI.PlayerIntent.Ranged"), false);
	TagIntentSkill = FGameplayTag::RequestGameplayTag(TEXT("AI.PlayerIntent.Skill"), false);
	TagDeflect = FGameplayTag::RequestGameplayTag(TEXT("AI.Enemy.Deflect"), false);
	TagExecution = FGameplayTag::RequestGameplayTag(TEXT("AI.Enemy.Execution"), false);
	TagCounterAttack = FGameplayTag::RequestGameplayTag(TEXT("AI.Enemy.CounterAttack"), false);
	TagCancel = FGameplayTag::RequestGameplayTag(TEXT("AI.Enemy.Cancel"), false);
	TagStaggered = FGameplayTag::RequestGameplayTag(TEXT("Status.Staggered"), false);
	TagInvincible = FGameplayTag::RequestGameplayTag(TEXT("Status.Invincible"), false);

	RefreshPlayerReference();
	ApplyCurrentConfig();
	InitAttributes();
	ResetCounterBar();

	// 死亡事件：自己 ASC 广播 OnActorDied 时通知状态树。
	if (UKnsAbilitySystemComponent* ASC = GetEnemyASC())
	{
		ASC->OnActorDied.AddDynamic(this, &URHEnemyAIComponent::HandleActorDied);
	}
}

void URHEnemyAIComponent::RefreshPlayerReference()
{
	if (PlayerTarget && bPlayerCombatBound)
	{
		return;
	}

	AActor* NewPlayer = nullptr;
	if (UWorld* World = GetWorld())
	{
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			NewPlayer = PC->GetPawn();
		}
	}

	if (NewPlayer == PlayerTarget)
	{
		return;
	}

	PlayerTarget = NewPlayer;
	PlayerASC = nullptr;
	bPlayerCombatBound = false;

	if (!PlayerTarget)
	{
		return;
	}

	if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(PlayerTarget))
	{
		PlayerASC = Cast<UKnsAbilitySystemComponent>(ASI->GetAbilitySystemComponent());
	}

	if (UKnsCombatComponent* PlayerCombat = PlayerTarget->FindComponentByClass<UKnsCombatComponent>())
	{
		PlayerCombat->OnHitApplied.AddDynamic(this, &URHEnemyAIComponent::HandlePlayerHitApplied);
		bPlayerCombatBound = true;
	}
}

void URHEnemyAIComponent::ApplyCurrentConfig()
{
	if (Definition)
	{
		Definition->GetResolvedConfig(CurrentPhaseIndex, CurrentConfig);
	}
	else
	{
		CurrentConfig = FRHEnemyRuntimeConfig();
	}

	if (AActor* Owner = GetOwner())
	{
		if (ACharacter* Character = Cast<ACharacter>(Owner))
		{
			if (UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement())
			{
				MoveComp->MaxWalkSpeed = CurrentConfig.Attributes.WalkSpeed;
				MoveComp->RotationRate.Yaw = CurrentConfig.Attributes.TurnSpeed;
			}
		}
	}
	if (ARHEnemyBase* Enemy = Cast<ARHEnemyBase>(GetOwner()))
	{
		Enemy->ApplyFloatBarConfig(CurrentConfig.bFloatBar);
	}
}

void URHEnemyAIComponent::InitAttributes()
{
	UKnsAbilitySystemComponent* ASC = GetEnemyASC();
	if (!ASC)
	{
		return;
	}

	if (UKnsCommonAttributeSet* CommonAS = const_cast<UKnsCommonAttributeSet*>(ASC->GetSet<UKnsCommonAttributeSet>()))
	{
		CommonAS->InitMaxHealth(CurrentConfig.Attributes.MaxHealth);
		if (!bAttributesInitialized)
		{
			CommonAS->InitHealth(CurrentConfig.Attributes.MaxHealth);
			bAttributesInitialized = true;
		}
		else
		{
			CommonAS->InitHealth(FMath::Clamp(CommonAS->GetHealth(), 1.f, CurrentConfig.Attributes.MaxHealth));
		}
		CommonAS->InitAttackPower(CurrentConfig.Attributes.AttackPower);
		CommonAS->InitMaxResonance(CurrentConfig.Resonance.MaxResonance);
		CommonAS->InitResonance(0.f);
	}
	if (ARHEnemyBase* Enemy = Cast<ARHEnemyBase>(GetOwner()))
	{
		Enemy->RefreshFloatPanel();
	}
}

void URHEnemyAIComponent::RefreshSnapshot()
{
	AActor* Owner = GetOwner();
	Snapshot.Player = PlayerTarget;
	Snapshot.Distance = 0.f;
	Snapshot.Distance2D = 0.f;
	Snapshot.AngleToPlayer = 0.f;
	Snapshot.bPlayerAttacking = false;
	Snapshot.PlayerBehavior = ERHPlayerBehavior::None;
	Snapshot.bPlayerBusy = false;
	Snapshot.bPlayerBlocking = false;
	Snapshot.bPlayerParrying = false;
	Snapshot.bPlayerDodging = false;
	Snapshot.bPlayerGuarding = false;
	Snapshot.bPlayerHealing = false;
	Snapshot.bPlayerOnAir = false;
	Snapshot.PlayerIntent = FGameplayTag();
	Snapshot.TimeSinceLastPlayerAttack = 0.f;
	Snapshot.PlayerHealthPercent = 100.f;
	Snapshot.PlayerWeaponId = NAME_None;

	if (!Owner || !PlayerTarget)
	{
		return;
	}

	const FVector Delta = PlayerTarget->GetActorLocation() - Owner->GetActorLocation();
	Snapshot.Distance = Delta.Size();
	Snapshot.Distance2D = Delta.Size2D();
	const FVector Dir2D = Delta.GetSafeNormal2D();
	if (!Dir2D.IsNearlyZero())
	{
		Snapshot.AngleToPlayer = FMath::RadiansToDegrees(
			FMath::Acos(FMath::Clamp(FVector::DotProduct(Owner->GetActorForwardVector(), Dir2D), -1.f, 1.f)));
	}

	if (UKnsAbilitySystemComponent* ASC = PlayerASC)
	{
		Snapshot.bPlayerGuarding = TagPlayerGuarding.IsValid() && ASC->HasMatchingGameplayTag(TagPlayerGuarding);
		Snapshot.bPlayerHealing = TagPlayerHealing.IsValid() && ASC->HasMatchingGameplayTag(TagPlayerHealing);
		Snapshot.bPlayerOnAir = ASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("State.Player.OnAir"), false));

		// 玩家行为：State.Player.<行为> 互斥（SetActionState 维护，进 Idle 全摘、切换先摘旧挂新）。
		const bool bHasAttacking = TagPlayerAttacking.IsValid() && ASC->HasMatchingGameplayTag(TagPlayerAttacking);
		const bool bHasSkill = TagPlayerSkill.IsValid() && ASC->HasMatchingGameplayTag(TagPlayerSkill);
		const bool bHasLoad = TagPlayerLoad.IsValid() && ASC->HasMatchingGameplayTag(TagPlayerLoad);
		const bool bHasToss = TagPlayerToss.IsValid() && ASC->HasMatchingGameplayTag(TagPlayerToss);
		const bool bHasDodge = TagPlayerDodge.IsValid() && ASC->HasMatchingGameplayTag(TagPlayerDodge);
		const bool bHasBlock = TagPlayerBlock.IsValid() && ASC->HasMatchingGameplayTag(TagPlayerBlock);
		const bool bHasParry = TagPlayerParry.IsValid() && ASC->HasMatchingGameplayTag(TagPlayerParry);

		Snapshot.bPlayerBusy = TagPlayerBusy.IsValid() && ASC->HasMatchingGameplayTag(TagPlayerBusy);
		Snapshot.bPlayerAttacking = bHasAttacking; // 规范：Attacking 只代表普攻。
		Snapshot.bPlayerBlocking = bHasBlock;
		Snapshot.bPlayerParrying = bHasParry;
		Snapshot.bPlayerDodging = bHasDodge;

		if (bHasBlock)
		{
			Snapshot.PlayerBehavior = ERHPlayerBehavior::Block;
		}
		else if (bHasParry)
		{
			Snapshot.PlayerBehavior = ERHPlayerBehavior::Parry;
		}
		else if (bHasDodge)
		{
			Snapshot.PlayerBehavior = ERHPlayerBehavior::Dodge;
		}
		else if (bHasSkill)
		{
			Snapshot.PlayerBehavior = ERHPlayerBehavior::Skill;
		}
		else if (bHasLoad)
		{
			Snapshot.PlayerBehavior = ERHPlayerBehavior::Load;
		}
		else if (bHasToss)
		{
			Snapshot.PlayerBehavior = ERHPlayerBehavior::Toss;
		}
		else if (bHasAttacking)
		{
			Snapshot.PlayerBehavior = ERHPlayerBehavior::Attacking;
		}

		if (Snapshot.bPlayerGuarding)
		{
			// 防御中：意图视为"无"（不是攻击），避免防御招式带的 Melee 意图 tag 误判。
			Snapshot.PlayerIntent = FGameplayTag();
		}
		else if (Snapshot.bPlayerHealing)
		{
			Snapshot.PlayerIntent = TagPlayerHealing;
		}
		else if (TagIntentRanged.IsValid() && ASC->HasMatchingGameplayTag(TagIntentRanged))
		{
			Snapshot.PlayerIntent = TagIntentRanged;
		}
		else if (TagIntentSkill.IsValid() && ASC->HasMatchingGameplayTag(TagIntentSkill))
		{
			Snapshot.PlayerIntent = TagIntentSkill;
		}
		else if (TagIntentMelee.IsValid() && ASC->HasMatchingGameplayTag(TagIntentMelee))
		{
			Snapshot.PlayerIntent = TagIntentMelee;
		}
	}

	if (PlayerASC)
	{
		const float MaxHealth = PlayerASC->GetAttributeValue(UKnsCommonAttributeSet::GetMaxHealthAttribute());
		const float CurrentHealth = PlayerASC->GetAttributeValue(UKnsCommonAttributeSet::GetHealthAttribute());
		Snapshot.PlayerHealthPercent = MaxHealth > 0.f
			? FMath::Clamp((CurrentHealth / MaxHealth) * 100.f, 0.f, 100.f)
			: 0.f;
	}

	if (UKnsCombatComponent* PlayerCombat = PlayerTarget->FindComponentByClass<UKnsCombatComponent>())
	{
		if (URHWeaponDefinition* WeaponDef = PlayerCombat->WeaponDefinition)
		{
			Snapshot.PlayerWeaponId = WeaponDef->WeaponId;
		}
	}

	const bool bPlayerAttackingNow = Snapshot.bPlayerAttacking;
	if (bPlayerAttackingNow && !bWasPlayerAttacking)
	{
		SendAIEvent(FGameplayTag::RequestGameplayTag(TEXT("AI.Player.AttackStarted"), false));
	}
	bWasPlayerAttacking = bPlayerAttackingNow;

	const bool bPlayerMeleeIntentNow = Snapshot.PlayerIntent == TagIntentMelee;
	if (bPlayerMeleeIntentNow && !bWasPlayerMeleeIntent)
	{
		SendAIEvent(FGameplayTag::RequestGameplayTag(TEXT("AI.Player.MeleeIntent"), false));
	}
	bWasPlayerMeleeIntent = bPlayerMeleeIntentNow;

	if (UWorld* World = GetWorld())
	{
		Snapshot.TimeSinceLastPlayerAttack = World->GetTimeSeconds() - LastPlayerAttackTime;
	}
}

void URHEnemyAIComponent::TickCounterBar(float DeltaTime)
{
	if (bResonanceBroken)
	{
		return;
	}

	// 玩家挂机时反击条自然衰减更快；玩家攻击中由命中扣减（HandlePlayerHitApplied）。
	// Counter bar no longer decays over time; it is drained only by landed player hits.
}

void URHEnemyAIComponent::TickCounterAttack(float DeltaTime)
{
	if (!bAwaitingCounterAttack)
	{
		return;
	}

	URHEnemyCombatComponent* Combat = GetEnemyCombatComponent();
	if (Combat && Combat->IsMontagePlaying())
	{
		// Deflect 蒙太奇还在播，等它结束。
		return;
	}

	if (!bCounterAttackTreeRestarted)
	{
		// 蒙太奇结束：先重启行为树，并等它稳定 Tick 一拍再发反攻事件，避免事件被树初始化吞掉。
		bCounterAttackTreeRestarted = true;
		bIsDeflecting = false;
		SetStateTreeLogicEnabled(true);
		CounterAttackDelayRemaining = 0.f;
		return;
	}

	CounterAttackDelayRemaining += DeltaTime;
	if (CounterAttackDelayRemaining >= 0.1f)
	{
		bAwaitingCounterAttack = false;
		bCounterAttackTreeRestarted = false;
		CounterAttackDelayRemaining = 0.f;
		SendAIEvent(TagCounterAttack);
	}
}

void URHEnemyAIComponent::TickResonance(float DeltaTime)
{
	UKnsAbilitySystemComponent* ASC = GetEnemyASC();
	if (!ASC)
	{
		return;
	}

	if (bResonanceBroken)
	{
		ASC->ApplyResonanceDelta(-CurrentConfig.Resonance.DecayPerSecondDuringBreak * DeltaTime);
		if (ASC->GetResonance() <= 0.f && !bExecutionInProgress)
		{
			HandleBreakDepleted();
		}
	}
	else
	{
		ASC->ApplyResonanceDelta(-CurrentConfig.Resonance.DecayPerSecond * DeltaTime);
	}
}

void URHEnemyAIComponent::TickBreakState(float DeltaTime)
{
	// 破防（处决等待）状态维护：原 StateTree Down 任务的职责搬到组件层（破防时树已停）。
	(void)DeltaTime;
	if (!bResonanceBroken || bExecutionInProgress)
	{
		return;
	}
	URHEnemyCombatComponent* Combat = GetEnemyCombatComponent();
	if (!Combat)
	{
		return;
	}
	// 倒地循环：当前无蒙太奇（受击动画播完/被打断）就重播 start 段，保持处决等待姿态。
	// 破防瞬间同帧的受击动画（HandleIncomingHit）会先播，下一帧检测到无蒙太奇再回倒地，与原 Down 任务一致。
	if (!Combat->IsMontagePlaying())
	{
		Combat->PlayMontageSection(CurrentConfig.DownMontage.LoadSynchronous(), TEXT("start"));
	}
}

void URHEnemyAIComponent::MaybeRequestExecution()
{
	if (!bResonanceBroken || bIsDowned || !PlayerTarget || bExecutionEventSent)
	{
		return;
	}
	if (!IsInExecutionRange(PlayerTarget))
	{
		return;
	}

	bool bHasAttackInput = Snapshot.bPlayerAttacking;
	if (!bHasAttackInput)
	{
		if (IRHCombatActionInterface* PlayerInterface = Cast<IRHCombatActionInterface>(PlayerTarget))
		{
			bHasAttackInput = PlayerInterface->HasPendingAction();
		}
	}
	if (!bHasAttackInput)
	{
		return;
	}

	StartPlayerExecution();
}

void URHEnemyAIComponent::RequestExecutionFromHit()
{
	// 破防中再次受击 → 直接进入处刑：玩家都已经打过来了，不再要求距离/按键检测。
	if (!bResonanceBroken || bExecutionInProgress || !PlayerTarget || bExecutionEventSent)
	{
		return;
	}
	StartPlayerExecution();
}

void URHEnemyAIComponent::StartPlayerExecution()
{
	// 破防时状态树已停，原来由 StateTree Execution 任务发起的处决直接在这里执行。
	// 先确认武器配了处决蒙太奇：缺则本轮不发起（下帧/下次受击重试），避免玩家蒙太奇播了敌人没反应卡死。
	URHWeaponDefinition* WeaponDef = nullptr;
	if (UKnsCombatComponent* PlayerCombat = PlayerTarget->FindComponentByClass<UKnsCombatComponent>())
	{
		WeaponDef = PlayerCombat->WeaponDefinition;
	}
	UAnimMontage* EnemyExecMontage = WeaponDef ? WeaponDef->ExecutedMontage.Get() : nullptr;
	if (!EnemyExecMontage)
	{
		return;
	}

	bExecutionEventSent = true;
	BeginExecution();

	const float ForwardDistance = WeaponDef ? WeaponDef->ExecutedDistance : 120.f;
	ARHEnemyBase* Pawn = Cast<ARHEnemyBase>(GetOwner());
	if (Pawn)
	{
		// 把玩家放到敌人正前方，双方都朝对方，保证两边蒙太奇对齐。
		const FVector EnemyLocation = Pawn->GetActorLocation();
		const FVector EnemyForward = Pawn->GetActorForwardVector().GetSafeNormal2D();
		const FVector TargetPlayerLocation = EnemyLocation + EnemyForward * ForwardDistance;
		PlayerTarget->SetActorLocation(TargetPlayerLocation, false, nullptr, ETeleportType::TeleportPhysics);

		const FVector ToEnemy = (EnemyLocation - PlayerTarget->GetActorLocation()).GetSafeNormal2D();
		if (!ToEnemy.IsNearlyZero())
		{
			PlayerTarget->SetActorRotation(ToEnemy.Rotation());
		}

		const FVector ToPlayer = (PlayerTarget->GetActorLocation() - EnemyLocation).GetSafeNormal2D();
		if (!ToPlayer.IsNearlyZero())
		{
			Pawn->SetActorRotation(ToPlayer.Rotation());
		}
	}

	if (IRHCombatActionInterface* PlayerInterface = Cast<IRHCombatActionInterface>(PlayerTarget))
	{
		PlayerInterface->StartExecution(Pawn);
	}
	if (URHEnemyCombatComponent* Combat = GetEnemyCombatComponent())
	{
		Combat->PlayExecutionMontage(EnemyExecMontage);
	}
}

void URHEnemyAIComponent::SetRotateToPlayer(bool bRotate)
{
	bRotateToPlayer = bRotate;
}

void URHEnemyAIComponent::TickRotateToPlayer(float DeltaTime)
{
	if (!bRotateToPlayer || !PlayerTarget)
	{
		return;
	}
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	const FVector Dir = (PlayerTarget->GetActorLocation() - Owner->GetActorLocation()).GetSafeNormal2D();
	if (Dir.IsNearlyZero())
	{
		return;
	}

	const FRotator TargetRot = Dir.Rotation();
	const float TurnSpeed = CurrentConfig.Attributes.TurnSpeed;
	if (TurnSpeed <= 0.f)
	{
		Owner->SetActorRotation(TargetRot);
	}
	else
	{
		Owner->SetActorRotation(FMath::RInterpTo(Owner->GetActorRotation(), TargetRot, DeltaTime, TurnSpeed));
	}
}

void URHEnemyAIComponent::SetStateTreeLogicEnabled(bool bEnabled)
{
	AAIController* Controller = nullptr;
	if (APawn* AsPawn = Cast<APawn>(GetOwner()))
	{
		Controller = Cast<AAIController>(AsPawn->GetController());
	}
	if (!Controller)
	{
		return;
	}
	if (UStateTreeAIComponent* ST = Controller->FindComponentByClass<UStateTreeAIComponent>())
	{
		if (bEnabled)
		{
			ST->StartLogic();
		}
		else
		{
			ST->StopLogic(TEXT("Enemy Deflect"));
		}
	}
}

void URHEnemyAIComponent::ResetCounterBar()
{
	CounterBarValue = CurrentConfig.CounterBar.Max;
}

float URHEnemyAIComponent::GetCounterBarPercent() const
{
	return CurrentConfig.CounterBar.Max > 0.f
		? FMath::Clamp(CounterBarValue / CurrentConfig.CounterBar.Max, 0.f, 1.f)
		: 0.f;
}

bool URHEnemyAIComponent::IsCounterBarEmpty() const
{
	return CounterBarValue <= 0.f;
}

void URHEnemyAIComponent::AddResonance(float Amount)
{
	if (Amount <= 0.f || bResonanceBroken)
	{
		return;
	}

	UKnsAbilitySystemComponent* ASC = GetEnemyASC();
	if (!ASC)
	{
		return;
	}

	ASC->ApplyResonanceDelta(Amount);
	if (ASC->GetResonance() >= ASC->GetMaxResonance())
	{
		EnterBreak();
	}
}

float URHEnemyAIComponent::GetResonancePercent() const
{
	UKnsAbilitySystemComponent* ASC = GetEnemyASC();
	if (!ASC)
	{
		return 0.f;
	}
	const float Max = ASC->GetMaxResonance();
	return Max > 0.f ? FMath::Clamp(ASC->GetResonance() / Max, 0.f, 1.f) : 0.f;
}

bool URHEnemyAIComponent::IsBroken() const
{
	return bResonanceBroken;
}

bool URHEnemyAIComponent::WasJustBroken() const
{
	return GFrameNumber == BreakTriggerFrame;
}

bool URHEnemyAIComponent::IsStaggered() const
{
	UKnsAbilitySystemComponent* ASC = GetEnemyASC();
	return ASC && TagStaggered.IsValid() && ASC->HasMatchingGameplayTag(TagStaggered);
}

float URHEnemyAIComponent::GetBreakDamageMultiplier() const
{
	return CurrentConfig.Resonance.BreakDamageMultiplier;
}

void URHEnemyAIComponent::ApplyBreakDamage(float Damage)
{
	// 注意：当前未被调用（KnsASC 处决等待保护已注释掉调用）。
	// 破防期间若直接扣共振到 0 会触发 HandleBreakDepleted 解除破防 + 播起身，
	// 同一帧受击打断会因 !IsBroken() 成立而发 HitTaken，把 Down（处决等待）态顶掉。
	// 如需恢复"破防中受击加速破防结束"，请确保解除时机与受击打断不冲突。
	if (!bResonanceBroken || Damage <= 0.f)
	{
		return;
	}

	UKnsAbilitySystemComponent* ASC = GetEnemyASC();
	if (!ASC)
	{
		return;
	}

	// 直接减，不是加快回落速率。
	ASC->ApplyResonanceDelta(-Damage);
	if (ASC->GetResonance() <= 0.f && !bExecutionInProgress)
	{
		HandleBreakDepleted();
	}
}

bool URHEnemyAIComponent::IsInExecutionRange(AActor* Source) const
{
	AActor* Owner = GetOwner();
	if (!Owner || !Source)
	{
		return false;
	}
	return (Source->GetActorLocation() - Owner->GetActorLocation()).Size2D() <= CurrentConfig.Attributes.ExecutionRange;
}

void URHEnemyAIComponent::ExecuteEnemy(AActor* Instigator)
{
	if (!bResonanceBroken)
	{
		return;
	}

	ApplyExecutionDamage();
	FinishExecution();
	UE_LOG(LogTemp, Warning, TEXT("RHEnemy: executed %s"), *GetNameSafe(GetOwner()));
}

bool URHEnemyAIComponent::DrainCounterBarOnPlayerHit(float Damage, AActor* Instigator)
{
	if (!bInitialized)
	{
		return false;
	}
	// 破防（处决等待）中不扣反击条：避免条空触发 Deflect（弹开 + 反攻计时）干扰处决等待。
	if (bResonanceBroken)
	{
		return false;
	}

	if (UWorld* World = GetWorld())
	{
		LastPlayerAttackTime = World->GetTimeSeconds();
	}

	if (bResonanceBroken)
	{
		return false;
	}

	if (Damage == 0.f)
	{
		return false;
	}

	if (CounterBarValue <= 0.f)
	{
		// 条形已在 0：不重复发 Deflect 事件，也不让同一状态持续吞掉后续命中。
		return false;
	}

	// 负数 Damage = 给敌人回反击条（CounterBarValue - 负数 = 增加）；统一 clamp 到 [0, Max]，回血不超过上限。
	CounterBarValue = FMath::Clamp(CounterBarValue - Damage, 0.f, CurrentConfig.CounterBar.Max);
	if (CounterBarValue <= 0.f)
	{
		TriggerDeflect(Instigator);
		return true;
	}
	return false;
}

void URHEnemyAIComponent::BeginExecution()
{
	// 幂等：处决进行中重复调用（玩家侧 TryStartExecution / 攻击输入同时触发）直接忽略。
	if (bExecutionInProgress)
	{
		return;
	}
	bExecutionDamageApplied = false;
	bExecutionInProgress = true;
	bPendingDeathAfterExecution = false;
	if (UKnsAbilitySystemComponent* ASC = GetEnemyASC())
	{
		ASC->ApplyResonanceDelta(-ASC->GetResonance());
	}
}

void URHEnemyAIComponent::ApplyExecutionDamage()
{
	if (!bResonanceBroken || bExecutionDamageApplied)
	{
		return;
	}

	const float FixedDamage = CurrentConfig.Resonance.ExecutionFixedDamage;
	if (PlayerASC)
	{
		PlayerASC->ApplyFlatDamageToActor(GetOwner(), FixedDamage);
	}
	bExecutionDamageApplied = true;
	UE_LOG(LogTemp, Warning, TEXT("RHEnemy: execution damage %.1f applied to %s"), FixedDamage, *GetNameSafe(GetOwner()));
}

void URHEnemyAIComponent::FinishExecution()
{
	if (!bResonanceBroken)
	{
		return;
	}

	ApplyExecutionDamage();
	bExecutionDamageApplied = false;

	// 处决中被打空血（HandleActorDied 已标记）：处决动画已自然播完，删武器并直接销毁，
	// 不再 ExitBreak 起身（不起身无敌、不重启树）。
	if (bPendingDeathAfterExecution)
	{
		bExecutionInProgress = false;
		bExecutionEventSent = false;
		if (URHEnemyCombatComponent* Combat = GetEnemyCombatComponent())
		{
			Combat->DestroySpawnedWeapon();
		}
		if (AActor* Owner = GetOwner())
		{
			Owner->Destroy();
		}
		return;
	}

	ExitBreak(true);
	bExecutionInProgress = false;
	bExecutionEventSent = false;
}

void URHEnemyAIComponent::OpenDeflectWindow()
{
	bDeflectWindowActive = true;
}

void URHEnemyAIComponent::CloseDeflectWindow()
{
	bDeflectWindowActive = false;
}

bool URHEnemyAIComponent::IsDeflectWindowActive() const
{
	return bDeflectWindowActive;
}

void URHEnemyAIComponent::ClearDeflectSucceeded()
{
	bDeflectSucceeded = false;
}

bool URHEnemyAIComponent::IsDeflectSucceeded() const
{
	return bDeflectSucceeded;
}

void URHEnemyAIComponent::NotifyDeflectSuccess(AActor* Instigator)
{
	bDeflectSucceeded = true;
	CloseDeflectWindow();

	if (PlayerTarget)
	{
		if (IRHCombatActionInterface* PlayerInterface = Cast<IRHCombatActionInterface>(PlayerTarget))
		{
			// 玩家被弹开：走接口播防御破防蒙太奇并进入受击硬直。
			PlayerInterface->HandleDeflected(GetOwner());
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("RHEnemy: deflect success, player staggered"));
}

void URHEnemyAIComponent::TriggerDeflect(AActor* Instigator)
{
	bIsDeflecting = true;
	// 冻结行为树：弹开期间不再让任何 state 抢逻辑，蒙太奇播完再恢复。
	SetStateTreeLogicEnabled(false);

	if (URHEnemyCombatComponent* Combat = GetEnemyCombatComponent())
	{
		UAnimMontage* DeflectMontage = CurrentConfig.DeflectMontage.LoadSynchronous();
		if (DeflectMontage)
		{
			Combat->PlayDeflectMontage(DeflectMontage);
		}
	}
	bAwaitingCounterAttack = true;
	CounterAttackDelayRemaining = 0.f;
	bCounterAttackTreeRestarted = false;

	AActor* Player = PlayerTarget ? PlayerTarget.Get() : Instigator;
	if (Player)
	{
		if (IRHCombatActionInterface* PlayerInterface = Cast<IRHCombatActionInterface>(Player))
		{
			PlayerInterface->HandleDeflected(GetOwner());
		}
	}

	ResetCounterBar();
	UE_LOG(LogTemp, Warning, TEXT("RHEnemy: counter bar broken, direct deflect triggered"));
}

void URHEnemyAIComponent::SetPhase(int32 NewPhase)
{
	const int32 ClampedPhase = FMath::Max(1, NewPhase);
	if (ClampedPhase == CurrentPhaseIndex)
	{
		return;
	}

	CurrentPhaseIndex = ClampedPhase;
	ApplyCurrentConfig();
	InitAttributes();
	ResetCounterBar();

	if (bResonanceBroken)
	{
		ExitBreak(false);
	}
	if (bIsDowned)
	{
		bIsDowned = false;
		EndGetupInvincible();
	}
	if (UKnsAbilitySystemComponent* ASC = GetEnemyASC())
	{
		ASC->ApplyResonanceDelta(-ASC->GetResonance());
	}

	UE_LOG(LogTemp, Warning, TEXT("RHEnemy: phase -> %d on %s"), CurrentPhaseIndex, *GetNameSafe(GetOwner()));
}

bool URHEnemyAIComponent::HasMoveSet() const
{
	// 近战有起手式 / 中间段 / 收尾式任一可用即视为有招式池。
	for (const FRHEnemyOpenerEntry& Entry : CurrentConfig.Openers)
	{
		if (Entry.Move)
		{
			return true;
		}
	}
	for (const FRHEnemyComboChain& Chain : CurrentConfig.MiddleMoves)
	{
		for (const FRHEnemyMoveEntry& Entry : Chain.Moves)
		{
			if (Entry.Move)
			{
				return true;
			}
		}
	}
	for (const FRHEnemyFinisherEntry& Entry : CurrentConfig.Finishers)
	{
		if (Entry.Move)
		{
			return true;
		}
	}
	return false;
}

bool URHEnemyAIComponent::PickCombo(TArray<URHEnemyMoveDefinition*>& OutChain) const
{
	OutChain.Reset();
	// 完整近战链 = 起手式 + 中间招式段 + 收尾式（各自按距离/权重选择）。
	if (URHEnemyMoveDefinition* Opener = PickOpener(CurrentConfig.Openers))
	{
		OutChain.Add(Opener);
	}
	TArray<URHEnemyMoveDefinition*> MiddleChain;
	if (PickComboFromSet(CurrentConfig.MiddleMoves, MiddleChain))
	{
		OutChain.Append(MiddleChain);
	}
	if (URHEnemyMoveDefinition* Finisher = PickFinisher(CurrentConfig.Finishers))
	{
		OutChain.Add(Finisher);
	}
	if (!OutChain.IsEmpty())
	{
		DebugPrint(FString::Printf(TEXT("PickCombo: %d 段（起手+中间+收尾）"), OutChain.Num()));
		return true;
	}
	return false;
}

URHEnemyMoveDefinition* URHEnemyAIComponent::PickOpener(const TArray<FRHEnemyOpenerEntry>& Openers) const
{
	const float Dist = Snapshot.Distance2D;
	auto IsInRange = [Dist](float MinRange, float MaxRange)
	{
		return Dist >= MinRange && (MaxRange < 0.f || Dist <= MaxRange);
	};

	TArray<const FRHEnemyOpenerEntry*> Eligible;
	float TotalWeight = 0.f;
	for (const FRHEnemyOpenerEntry& Entry : Openers)
	{
		if (Entry.Move && IsInRange(Entry.MinRange, Entry.MaxRange))
		{
			Eligible.Add(&Entry);
			TotalWeight += FMath::Max(0.f, Entry.Weight);
		}
	}
	// 只考虑符合距离的起手式：无符合距离 → 返回 nullptr（不回退全池，让 Aggressive 直接 Failed）。
	if (TotalWeight <= 0.f || Eligible.IsEmpty())
	{
		return nullptr;
	}

	float Roll = FMath::FRandRange(0.f, TotalWeight);
	for (const FRHEnemyOpenerEntry* Entry : Eligible)
	{
		Roll -= FMath::Max(0.f, Entry->Weight);
		if (Roll <= 0.f)
		{
			return Entry->Move;
		}
	}
	return Eligible.Last()->Move;
}

URHEnemyMoveDefinition* URHEnemyAIComponent::PickFinisher(const TArray<FRHEnemyFinisherEntry>& Finishers) const
{
	const float Dist = Snapshot.Distance2D;
	auto IsInRange = [Dist](float MinRange, float MaxRange)
	{
		return Dist >= MinRange && (MaxRange < 0.f || Dist <= MaxRange);
	};

	TArray<const FRHEnemyFinisherEntry*> Eligible;
	float TotalWeight = 0.f;
	for (const FRHEnemyFinisherEntry& Entry : Finishers)
	{
		if (Entry.Move && IsInRange(Entry.MinRange, Entry.MaxRange))
		{
			Eligible.Add(&Entry);
			TotalWeight += FMath::Max(0.f, Entry.Weight);
		}
	}
	// 只考虑符合距离的收尾式：无符合距离 → 返回 nullptr（不回退全池，让 Aggressive 直接 Failed）。
	if (TotalWeight <= 0.f || Eligible.IsEmpty())
	{
		return nullptr;
	}

	float Roll = FMath::FRandRange(0.f, TotalWeight);
	for (const FRHEnemyFinisherEntry* Entry : Eligible)
	{
		Roll -= FMath::Max(0.f, Entry->Weight);
		if (Roll <= 0.f)
		{
			return Entry->Move;
		}
	}
	return Eligible.Last()->Move;
}

URHEnemyMoveDefinition* URHEnemyAIComponent::PickRemoteMove(const TArray<TObjectPtr<URHEnemyMoveDefinition>>& RemoteMoves) const
{
	TArray<URHEnemyMoveDefinition*> Valid;
	for (const TObjectPtr<URHEnemyMoveDefinition>& Move : RemoteMoves)
	{
		if (Move)
		{
			Valid.Add(Move);
		}
	}
	if (Valid.IsEmpty())
	{
		return nullptr;
	}
	return Valid[FMath::RandRange(0, Valid.Num() - 1)];
}

bool URHEnemyAIComponent::IsSpecialAvailable() const
{
	if (SpecialCooldownRemaining > 0.f || CurrentConfig.SpecialMoves.IsEmpty())
	{
		return false;
	}
	const float Dist = Snapshot.Distance2D;
	for (const FRHEnemySpecialEntry& Entry : CurrentConfig.SpecialMoves)
	{
		if (Entry.Action && Dist >= Entry.MinRange && (Entry.MaxRange < 0.f || Dist <= Entry.MaxRange))
		{
			return true;
		}
	}
	return false;
}

URHEnemyActionDefinition* URHEnemyAIComponent::PickSpecial()
{
	const float Dist = Snapshot.Distance2D;
	auto IsInRange = [Dist](float MinRange, float MaxRange)
	{
		return Dist >= MinRange && (MaxRange < 0.f || Dist <= MaxRange);
	};

	TArray<const FRHEnemySpecialEntry*> Eligible;
	float TotalWeight = 0.f;
	for (const FRHEnemySpecialEntry& Entry : CurrentConfig.SpecialMoves)
	{
		if (Entry.Action && IsInRange(Entry.MinRange, Entry.MaxRange))
		{
			Eligible.Add(&Entry);
			TotalWeight += FMath::Max(0.f, Entry.Weight);
		}
	}

	// 没有距离匹配：回退全池。
	if (TotalWeight <= 0.f)
	{
		for (const FRHEnemySpecialEntry& Entry : CurrentConfig.SpecialMoves)
		{
			if (Entry.Action)
			{
				Eligible.Add(&Entry);
				TotalWeight += FMath::Max(0.f, Entry.Weight);
			}
		}
	}
	if (TotalWeight <= 0.f)
	{
		return nullptr;
	}

	float Roll = FMath::FRandRange(0.f, TotalWeight);
	for (const FRHEnemySpecialEntry* Entry : Eligible)
	{
		Roll -= FMath::Max(0.f, Entry->Weight);
		if (Roll <= 0.f)
		{
			SpecialCooldownRemaining = Entry->CooldownSeconds;
			DebugPrint(FString::Printf(TEXT("PickSpecial: %s"), *GetNameSafe(Entry->Action.Get())));
			return Entry->Action;
		}
	}
	if (!Eligible.IsEmpty())
	{
		SpecialCooldownRemaining = Eligible.Last()->CooldownSeconds;
		DebugPrint(FString::Printf(TEXT("PickSpecial: %s"), *GetNameSafe(Eligible.Last()->Action.Get())));
		return Eligible.Last()->Action;
	}
	return nullptr;
}

bool URHEnemyAIComponent::PickComboFromSet(const TArray<FRHEnemyComboChain>& MoveSet, TArray<URHEnemyMoveDefinition*>& OutChain) const
{
	const float Dist = Snapshot.Distance2D;
	auto IsInRange = [Dist](float MinRange, float MaxRange)
	{
		return Dist >= MinRange && (MaxRange < 0.f || Dist <= MaxRange);
	};

	TArray<const FRHEnemyComboChain*> Eligible;
	float TotalWeight = 0.f;
	for (const FRHEnemyComboChain& Chain : MoveSet)
	{
		if (!Chain.Moves.IsEmpty() && IsInRange(Chain.MinRange, Chain.MaxRange))
		{
			Eligible.Add(&Chain);
			TotalWeight += FMath::Max(0.f, Chain.Weight);
		}
	}
	// 只考虑符合距离的段：没有符合距离的招式 → 返回 false（不回退全池，让 Aggressive 直接 Failed）。
	if (TotalWeight <= 0.f)
	{
		return false;
	}

	float Roll = FMath::FRandRange(0.f, TotalWeight);
	for (const FRHEnemyComboChain* Chain : Eligible)
	{
		Roll -= FMath::Max(0.f, Chain->Weight);
		if (Roll <= 0.f)
		{
			OutChain.Reset();
			for (const FRHEnemyMoveEntry& Entry : Chain->Moves)
			{
				if (Entry.Move)
				{
					OutChain.Add(Entry.Move);
				}
			}
			return !OutChain.IsEmpty();
		}
	}
	if (!Eligible.IsEmpty())
	{
		OutChain.Reset();
		for (const FRHEnemyMoveEntry& Entry : Eligible.Last()->Moves)
		{
			if (Entry.Move)
			{
				OutChain.Add(Entry.Move);
			}
		}
		return !OutChain.IsEmpty();
	}
	return false;
}

bool URHEnemyAIComponent::IsSpecialAvailableFromSet(const TArray<FRHEnemySpecialEntry>& Specials) const
{
	if (SpecialCooldownRemaining > 0.f || Specials.IsEmpty())
	{
		return false;
	}
	const float Dist = Snapshot.Distance2D;
	for (const FRHEnemySpecialEntry& Entry : Specials)
	{
		if (Entry.Action && Dist >= Entry.MinRange && (Entry.MaxRange < 0.f || Dist <= Entry.MaxRange))
		{
			return true;
		}
	}
	return false;
}

URHEnemyActionDefinition* URHEnemyAIComponent::PickSpecialFromSet(const TArray<FRHEnemySpecialEntry>& Specials)
{
	const float Dist = Snapshot.Distance2D;
	auto IsInRange = [Dist](float MinRange, float MaxRange)
	{
		return Dist >= MinRange && (MaxRange < 0.f || Dist <= MaxRange);
	};

	TArray<const FRHEnemySpecialEntry*> Eligible;
	float TotalWeight = 0.f;
	for (const FRHEnemySpecialEntry& Entry : Specials)
	{
		if (Entry.Action && IsInRange(Entry.MinRange, Entry.MaxRange))
		{
			Eligible.Add(&Entry);
			TotalWeight += FMath::Max(0.f, Entry.Weight);
		}
	}
	if (TotalWeight <= 0.f)
	{
		for (const FRHEnemySpecialEntry& Entry : Specials)
		{
			if (Entry.Action)
			{
				Eligible.Add(&Entry);
				TotalWeight += FMath::Max(0.f, Entry.Weight);
			}
		}
	}
	if (TotalWeight <= 0.f)
	{
		return nullptr;
	}

	float Roll = FMath::FRandRange(0.f, TotalWeight);
	for (const FRHEnemySpecialEntry* Entry : Eligible)
	{
		Roll -= FMath::Max(0.f, Entry->Weight);
		if (Roll <= 0.f)
		{
			SpecialCooldownRemaining = Entry->CooldownSeconds;
			return Entry->Action;
		}
	}
	if (!Eligible.IsEmpty())
	{
		SpecialCooldownRemaining = Eligible.Last()->CooldownSeconds;
		return Eligible.Last()->Action;
	}
	return nullptr;
}

void URHEnemyAIComponent::NotifyRemoteUsed()
{
	RemoteCooldownRemaining = CurrentConfig.RemoteCooldownSeconds;
}

void URHEnemyAIComponent::HandlePlayerHitApplied(AActor* Target, const FRHHitData& HitData)
{
	if (!bInitialized || Target != GetOwner())
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		LastPlayerAttackTime = World->GetTimeSeconds();
	}

	if (bResonanceBroken)
	{
		// 破防中：普攻在范围内走处决（KnsASC 已拦截）；战技继续灌伤（伤害系数在 KnsASC 结算）。
		return;
	}

	// 反击条扣减统一在 ApplyHitToActor 内通过 DrainCounterBarOnPlayerHit 处理。
}

void URHEnemyAIComponent::EnterBreak()
{
	if (bResonanceBroken)
	{
		return;
	}

	bResonanceBroken = true;
	bExecutionEventSent = false;
	bExecutionInProgress = false;
	// 记录触发帧：本帧（同一 ApplyHitToActor 流程）内后续的受击广播 = 触发破防那一下，不处决；
	// 跨帧的再次受击才算"破防后再次受击"，由 HandleEnemyHitReceived 直接处刑。
	BreakTriggerFrame = GFrameNumber;

	// 停状态树：破防（处决等待）期间树不再驱动 AI，倒地循环/处决/起身全由组件层管理，
	// 共振回落完（HandleBreakDepleted）或处决完成（FinishExecution）后再重启。
	SetStateTreeLogicEnabled(false);
	// 破防倒地：不再转向玩家（原 Down 任务职责）。
	SetRotateToPlayer(false);
	// 开处决范围球：玩家走进范围即可处决（Overlap 会设置玩家 ExecutionAvailable）。
	if (ARHEnemyBase* EnemyPawn = Cast<ARHEnemyBase>(GetOwner()))
	{
		EnemyPawn->SetExecutionRangeActive(true);
	}
	// 清反攻计时：避免破防前 Deflect 残留的 CounterAttack 事件在树重启时误触发。
	bAwaitingCounterAttack = false;
	bCounterAttackTreeRestarted = false;
	CounterAttackDelayRemaining = 0.f;

	if (UKnsAbilitySystemComponent* ASC = GetEnemyASC())
	{
		if (TagStaggered.IsValid() && !ASC->HasMatchingGameplayTag(TagStaggered))
		{
			ASC->AddLooseGameplayTag(TagStaggered);
		}
		ASC->OnResonanceBroken.Broadcast(GetOwner());
	}
	// 不再发 AI.Enemy.Break 事件：树已停，事件会积压到下次 StartLogic 误触发；倒地循环由 TickBreakState 维护。
	if (URHEnemyCombatComponent* Combat = GetEnemyCombatComponent())
	{
		Combat->TriggerBreakTimeDilation();
	}
	UE_LOG(LogTemp, Warning, TEXT("RHEnemy: resonance break on %s"), *GetNameSafe(GetOwner()));
}

void URHEnemyAIComponent::ExitBreak(bool bStartGetupInvincible)
{
	if (!bResonanceBroken)
	{
		return;
	}

	bResonanceBroken = false;
	bIsDowned = bStartGetupInvincible;
	ResetCounterBar();

	if (UKnsAbilitySystemComponent* ASC = GetEnemyASC())
	{
		ASC->ApplyResonanceDelta(-ASC->GetResonance());
		if (TagStaggered.IsValid())
		{
			ASC->RemoveLooseGameplayTag(TagStaggered);
		}
	}

	// 破防解除：关处决范围、恢复转向、重启状态树（回到正常 AI）。
	if (ARHEnemyBase* EnemyPawn = Cast<ARHEnemyBase>(GetOwner()))
	{
		EnemyPawn->SetExecutionRangeActive(false);
	}
	SetRotateToPlayer(true);
	SetStateTreeLogicEnabled(true);

	if (bStartGetupInvincible)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(
				GetupInvincibleTimerHandle,
				this,
				&URHEnemyAIComponent::EndGetupInvincible,
				CurrentConfig.Resonance.GetupInvincibleSeconds,
				false);
		}
	}
}

void URHEnemyAIComponent::HandleBreakDepleted()
{
	if (!bResonanceBroken)
	{
		return;
	}

	bExecutionEventSent = false;
	// 共振归零：播倒地蒙太奇 end 段（起身），然后解除破防。
	if (URHEnemyCombatComponent* Combat = GetEnemyCombatComponent())
	{
		Combat->PlayMontageSection(CurrentConfig.DownMontage.LoadSynchronous(), TEXT("end"));
	}
	ExitBreak(false);
}

void URHEnemyAIComponent::EndGetupInvincible()
{
	bIsDowned = false;
}

bool URHEnemyAIComponent::CanDodge() const
{
	if (LastDodgeTime < 0.f)
	{
		return true;
	}

	const UWorld* World = GetWorld();
	if (!World)
	{
		return true;
	}
	return World->GetTimeSeconds() - LastDodgeTime >= CurrentConfig.Feel.DodgeCooldownSeconds;
}

void URHEnemyAIComponent::NotifyDodgeStarted()
{
	if (UWorld* World = GetWorld())
	{
		LastDodgeTime = World->GetTimeSeconds();
	}
}

void URHEnemyAIComponent::SendAIEvent(FGameplayTag Tag)
{
	if (!Tag.IsValid())
	{
		return;
	}

	AAIController* Controller = nullptr;
	if (APawn* Pawn = Cast<APawn>(GetOwner()))
	{
		Controller = Cast<AAIController>(Pawn->GetController());
	}
	if (!Controller)
	{
		return;
	}

	if (UStateTreeAIComponent* ST = Controller->FindComponentByClass<UStateTreeAIComponent>())
	{
		ST->SendStateTreeEvent(Tag);
	}
	DebugPrint(FString::Printf(TEXT("Event: %s"), *Tag.ToString()));
}

bool URHEnemyAIComponent::IsAttacking() const
{
	URHEnemyCombatComponent* Combat = GetEnemyCombatComponent();
	return Combat && Combat->IsBusy();
}

void URHEnemyAIComponent::DebugPrint(const FString& Message) const
{
	const FString Line = FString::Printf(TEXT("[AI] %s"), *Message);
	UE_LOG(LogTemp, Log, TEXT("%s"), *Line);
	if (bShowDebugPrints && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Yellow, Line);
	}
}

void URHEnemyAIComponent::HandleActorDied(AActor* Actor)
{
	if (Actor != GetOwner())
	{
		return;
	}

	// 无论哪种死亡：先隐藏全部 UI（锁定白点/浮层血条），再通知玩家解除锁定/清敌人信息 HUD。
	if (ARHEnemyBase* Enemy = Cast<ARHEnemyBase>(GetOwner()))
	{
		Enemy->HideAllUI();
	}
	if (AActor* Player = GetPlayerTarget())
	{
		if (IRHCombatActionInterface* PlayerInterface = Cast<IRHCombatActionInterface>(Player))
		{
			PlayerInterface->NotifyEnemyDefeated(GetOwner());
		}
	}

	// 破防/处决中死亡（处决伤害打空血条）：状态树已停，且不需要死亡蒙太奇。
	// 不立即 Destroy——标记后等处决动画自然播完（FinishExecution）再删武器+销毁，
	// 避免处决动画被掐断；武器 actor 与敌人一起在 FinishExecution 销毁。
	if (bResonanceBroken || bExecutionInProgress)
	{
		bPendingDeathAfterExecution = true;
		return;
	}

	// 普通死亡：不依赖树的 Death 状态（树可能未响应 AI.Enemy.Death 事件导致血空不死），
	// 组件层直接接管——停树防继续行动 + 播死亡蒙太奇，播完 blend out 冻结动画并销毁。
	SetStateTreeLogicEnabled(false);
	SetRotateToPlayer(false);
	if (APawn* Pawn = Cast<APawn>(GetOwner()))
	{
		if (AAIController* Controller = Cast<AAIController>(Pawn->GetController()))
		{
			Controller->StopMovement();
		}
	}

	if (URHEnemyCombatComponent* Combat = GetEnemyCombatComponent())
	{
		Combat->StopCurrentMoveMontage();
		Combat->DestroySpawnedWeapon(); // 死亡时把武器一起清除
		Combat->TriggerBreakTimeDilation(); // 死亡时缓（破防同款 0.3x / 0.2s）
		UAnimMontage* DeathMontage = CurrentConfig.DeathMontage.LoadSynchronous();
		if (!Combat->PlayDeathMontage(DeathMontage))
		{
			// 未配置死亡蒙太奇或播放失败：直接销毁。
			GetOwner()->Destroy();
		}
	}
	else
	{
		GetOwner()->Destroy();
	}
}

URHEnemyCombatComponent* URHEnemyAIComponent::GetEnemyCombatComponent() const
{
	AActor* Owner = GetOwner();
	return Owner ? Owner->FindComponentByClass<URHEnemyCombatComponent>() : nullptr;
}

UKnsAbilitySystemComponent* URHEnemyAIComponent::GetEnemyASC() const
{
	AActor* Owner = GetOwner();
	if (Owner)
	{
		if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Owner))
		{
			return Cast<UKnsAbilitySystemComponent>(ASI->GetAbilitySystemComponent());
		}
	}
	return nullptr;
}
