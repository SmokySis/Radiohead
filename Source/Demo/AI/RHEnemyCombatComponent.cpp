#include "RHEnemyCombatComponent.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Demo/AI/RHEnemyActionDefinition.h"
#include "Demo/AI/RHEnemyAIComponent.h"
#include "Demo/AI/RHEnemyMoveDefinition.h"
#include "Demo/Character/BaseCharacter.h"
#include "Demo/Character/RHTestDummy.h"
#include "Demo/Combat/KnsCombatContextComponent.h"
#include "Demo/Combat/RHHitData.h"
#include "Demo/Combat/Weapon/AWeaponBase.h"
#include "Demo/GAS/KnsAbilitySystemComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

URHEnemyCombatComponent::URHEnemyCombatComponent()
{
	// 敌人属性由 URHEnemyAIComponent 从敌人 DA 初始化，不走 BP 默认值。
	AttributeInitializer.bApplyOnBeginPlay = false;
}

void URHEnemyCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	if (ABaseCharacter* Character = Cast<ABaseCharacter>(GetOwner()))
	{
		if (UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance())
		{
			AnimInstance->OnMontageEnded.AddDynamic(this, &URHEnemyCombatComponent::HandleEnemyActionMontageEnded);
			AnimInstance->OnMontageBlendingOut.AddDynamic(this, &URHEnemyCombatComponent::HandleDeathMontageBlendingOut);
		}
	}

	OnHitReceived.AddDynamic(this, &URHEnemyCombatComponent::HandleEnemyHitReceived);
}

void URHEnemyCombatComponent::TriggerBreakTimeDilation()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	UGameplayStatics::SetGlobalTimeDilation(World, BreakTimeDilationScale);
	World->GetTimerManager().ClearTimer(BreakTimeDilationTimerHandle);
	World->GetTimerManager().SetTimer(
		BreakTimeDilationTimerHandle,
		[World]()
		{
			if (World)
			{
				UGameplayStatics::SetGlobalTimeDilation(World, 1.f);
			}
		},
		BreakTimeDilationDuration,
		false);
}

bool URHEnemyCombatComponent::PlayMove(URHEnemyMoveDefinition* Move)
{
	if (!Move)
	{
		return false;
	}

	UAnimMontage* Montage = Move->Montage.LoadSynchronous();
	if (!Montage)
	{
		return false;
	}

	ABaseCharacter* Character = Cast<ABaseCharacter>(GetOwner());
	UAnimInstance* AnimInstance = Character ? Character->GetMesh()->GetAnimInstance() : nullptr;
	if (!Character || !AnimInstance)
	{
		return false;
	}

	// 连段衔接：直接播下一段覆盖上一段，避免先 Stop 回到 Idle 再开播。
	const bool bSameMontage = (CurrentEnemyMontage == Montage);

	CurrentEnemyMove = Move;
	CurrentEnemyAction = nullptr;
	CurrentEnemyMontage = Montage;
	bEnemyComboWindowOpen = false;
	bActionInterrupted = false;

	if (CombatContext)
	{
		CombatContext->SetPoiseState(0, Move->PoiseLevel, 0.f);
	}

	if (bSameMontage)
	{
		if (Move->SectionName != NAME_None)
		{
			if (AnimInstance->Montage_IsActive(Montage))
			{
				AnimInstance->Montage_JumpToSection(Move->SectionName, Montage);
			}
			else
			{
				AnimInstance->Montage_Play(Montage, Move->PlayRate);
				AnimInstance->Montage_JumpToSection(Move->SectionName, Montage);
			}
		}
		else
		{
			AnimInstance->Montage_Play(Montage, Move->PlayRate);
		}
		return true;
	}

	Character->PlayAnimMontage(Montage, Move->PlayRate, Move->SectionName);
	return true;
}

bool URHEnemyCombatComponent::PlayAction(URHEnemyActionDefinition* Action)
{
	if (!Action)
	{
		return false;
	}

	UAnimMontage* Montage = Action->Montage.LoadSynchronous();
	if (!Montage)
	{
		return false;
	}

	ABaseCharacter* Character = Cast<ABaseCharacter>(GetOwner());
	UAnimInstance* AnimInstance = Character ? Character->GetMesh()->GetAnimInstance() : nullptr;
	if (!Character || !AnimInstance)
	{
		return false;
	}

	// 与连段接段（PlayMove）一致：直接播新蒙太奇覆盖（引擎自动 blend），
	// 不手动 Montage_Stop 旧蒙太奇——手动停会先 blend 回 Idle 再播新蒙太奇，衔接卡顿。
	CurrentEnemyMove = nullptr;
	CurrentEnemyAction = Action;
	CurrentEnemyMontage = Montage;
	bEnemyComboWindowOpen = false;
	bActionInterrupted = false;

	// 特殊招式：激活武器拖尾（Niagara 资产在组件上配置）。
	CurrentCastPolarity = Action->Polarity;
	ActivateCastTrail();

	if (CombatContext)
	{
		CombatContext->SetPoiseState(0, Action->PoiseLevel, 0.f);
	}

	Character->PlayAnimMontage(Montage, Action->PlayRate, Action->SectionName);
	// 特殊招式施放：播一次 mesh 附属闪光。
	PlayCastFlash();
	return true;
}

bool URHEnemyCombatComponent::PlayMontage(UAnimMontage* Montage, float PlayRate)
{
	if (!Montage)
	{
		return false;
	}

	ABaseCharacter* Character = Cast<ABaseCharacter>(GetOwner());
	UAnimInstance* AnimInstance = Character ? Character->GetMesh()->GetAnimInstance() : nullptr;
	if (!Character || !AnimInstance)
	{
		return false;
	}

	// 状态级蒙太奇（格挡/弹开/倒地/起身/闪避等）：同样直接覆盖播，不手动停旧蒙太奇（避免回 Idle 再 blend 卡顿）。
	CurrentEnemyMove = nullptr;
	CurrentEnemyAction = nullptr;
	CurrentEnemyMontage = Montage;
	bEnemyComboWindowOpen = false;
	bActionInterrupted = false;

	if (CombatContext)
	{
		CombatContext->SetPoiseState(0, 0, 0.f);
	}

	Character->PlayAnimMontage(Montage, PlayRate);
	return true;
}

bool URHEnemyCombatComponent::PlayDeathMontage(UAnimMontage* Montage)
{
	if (!Montage)
	{
		return false;
	}

	ABaseCharacter* Character = Cast<ABaseCharacter>(GetOwner());
	UAnimInstance* AnimInstance = Character ? Character->GetMesh()->GetAnimInstance() : nullptr;
	if (!Character || !AnimInstance)
	{
		return false;
	}

	// 与 PlayMontage 一致直接覆盖播（不先 Stop 旧蒙太奇）。
	bIsDeathMontage = true;
	LastDeathMontage = Montage;
	CurrentEnemyMove = nullptr;
	CurrentEnemyAction = nullptr;
	CurrentEnemyMontage = Montage;
	bEnemyComboWindowOpen = false;
	bActionInterrupted = false;

	if (CombatContext)
	{
		CombatContext->SetPoiseState(0, 0, 0.f);
	}

	Character->PlayAnimMontage(Montage);
	return true;
}

void URHEnemyCombatComponent::HandleDeathMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted)
{
	// 只认当前死亡蒙太奇自己的 blend out（被打断/自然播完都算），防止其它蒙太奇误判销毁。
	if (!bIsDeathMontage || Montage != LastDeathMontage)
	{
		return;
	}

	// 死亡蒙太奇自然播完开始 blend out：立刻冻结骨骼网格动画，保持死亡姿势，
	// 否则 blend out 期间会插值回 Idle 站姿，出现“死后又站起来”。
	bIsDeathMontage = false;
	LastDeathMontage = nullptr;
	if (ABaseCharacter* Character = Cast<ABaseCharacter>(GetOwner()))
	{
		if (USkeletalMeshComponent* Mesh = Character->GetMesh())
		{
			Mesh->bPauseAnims = true;
		}
	}

	// 死亡蒙太奇已播完（或被打断）：保持死亡姿势并立即销毁。
	// 死亡流程由 AI 组件接管（HandleActorDied），不再依赖树的 Death Tick。
	if (AActor* Owner = GetOwner())
	{
		Owner->Destroy();
	}
}

bool URHEnemyCombatComponent::PlayMontageSection(UAnimMontage* Montage, FName SectionName, float PlayRate)
{
	if (!Montage)
	{
		return false;
	}

	ABaseCharacter* Character = Cast<ABaseCharacter>(GetOwner());
	UAnimInstance* AnimInstance = Character ? Character->GetMesh()->GetAnimInstance() : nullptr;
	if (!Character || !AnimInstance)
	{
		return false;
	}

	// 状态级蒙太奇的 section 播放（闪避 F/L/R/B、倒地 start/loop/end 等）：直接覆盖播，不手动停旧蒙太奇。
	CurrentEnemyMove = nullptr;
	CurrentEnemyAction = nullptr;
	CurrentEnemyMontage = Montage;
	bEnemyComboWindowOpen = false;
	bActionInterrupted = false;

	if (CombatContext)
	{
		CombatContext->SetPoiseState(0, 0, 0.f);
	}

	if (SectionName == NAME_None)
	{
		Character->PlayAnimMontage(Montage, PlayRate);
	}
	else
	{
		Character->PlayAnimMontage(Montage, PlayRate, SectionName);
	}
	return true;
}

void URHEnemyCombatComponent::StopCurrentMoveMontage()
{
	if (!CurrentEnemyMontage)
	{
		return;
	}

	UAnimMontage* OldMontage = CurrentEnemyMontage;
	CurrentEnemyMontage = nullptr;
	CurrentEnemyMove = nullptr;
	CurrentEnemyAction = nullptr;
	bEnemyComboWindowOpen = false;
	CurrentCastPolarity = ERHOnomPolarity::None;
	DeactivateCastTrail();

	if (ABaseCharacter* Character = Cast<ABaseCharacter>(GetOwner()))
	{
		if (UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance())
		{
			AnimInstance->Montage_Stop(0.1f, OldMontage);
		}
	}

	if (CombatContext)
	{
		CombatContext->SetPoiseState(0, 0, 0.f);
	}
}

void URHEnemyCombatComponent::SetComboWindowOpen(bool bOpen)
{
	bEnemyComboWindowOpen = bOpen;
}

void URHEnemyCombatComponent::PlayExecutionMontage(UAnimMontage* Montage)
{
	if (!Montage)
	{
		return;
	}

	ABaseCharacter* Character = Cast<ABaseCharacter>(GetOwner());
	UAnimInstance* AnimInstance = Character ? Character->GetMesh()->GetAnimInstance() : nullptr;
	if (!Character || !AnimInstance)
	{
		return;
	}

	if (CurrentEnemyMontage && CurrentEnemyMontage != Montage)
	{
		AnimInstance->Montage_Stop(0.05f, CurrentEnemyMontage);
	}

	CurrentEnemyMove = nullptr;
	CurrentEnemyAction = nullptr;
	CurrentEnemyMontage = Montage;
	bEnemyComboWindowOpen = false;
	bActionInterrupted = false;
	bIsExecutionMontage = true;

	if (CombatContext)
	{
		CombatContext->SetPoiseState(0, 0, 0.f);
	}

	Character->PlayAnimMontage(Montage);
}

void URHEnemyCombatComponent::PlayGetupMontage(UAnimMontage* Montage)
{
	if (!Montage)
	{
		return;
	}

	ABaseCharacter* Character = Cast<ABaseCharacter>(GetOwner());
	UAnimInstance* AnimInstance = Character ? Character->GetMesh()->GetAnimInstance() : nullptr;
	if (!Character || !AnimInstance)
	{
		return;
	}

	if (CurrentEnemyMontage && CurrentEnemyMontage != Montage)
	{
		AnimInstance->Montage_Stop(0.05f, CurrentEnemyMontage);
	}

	CurrentEnemyMove = nullptr;
	CurrentEnemyAction = nullptr;
	CurrentEnemyMontage = Montage;
	bEnemyComboWindowOpen = false;
	// 标记前一段动作被打断：让 Attack/Special 任务像受击一样自动失败退出状态，
	// 而 Deflect 蒙太奇仍直接叠播（不回 Idle），无需依赖 Deflect 事件接线。
	bActionInterrupted = true;
	bIsExecutionMontage = false;
	bIsGetupMontage = true;

	if (CombatContext)
	{
		CombatContext->SetPoiseState(0, 0, 0.f);
	}

	Character->PlayAnimMontage(Montage);
}

void URHEnemyCombatComponent::PlayDodgeBackMontage(UAnimMontage* Montage)
{
	if (!Montage)
	{
		return;
	}

	ABaseCharacter* Character = Cast<ABaseCharacter>(GetOwner());
	UAnimInstance* AnimInstance = Character ? Character->GetMesh()->GetAnimInstance() : nullptr;
	if (!Character || !AnimInstance)
	{
		return;
	}

	if (CurrentEnemyMontage && CurrentEnemyMontage != Montage)
	{
		AnimInstance->Montage_Stop(0.05f, CurrentEnemyMontage);
	}

	CurrentEnemyMove = nullptr;
	CurrentEnemyAction = nullptr;
	CurrentEnemyMontage = Montage;
	bEnemyComboWindowOpen = false;
	bActionInterrupted = false;
	bIsExecutionMontage = false;
	bIsGetupMontage = true;

	if (CombatContext)
	{
		CombatContext->SetPoiseState(0, 0, 0.f);
	}

	AnimInstance->Montage_Play(Montage);
	AnimInstance->Montage_JumpToSection(TEXT("B"), Montage);
}

void URHEnemyCombatComponent::PlayDeflectMontage(UAnimMontage* Montage)
{
	if (!Montage)
	{
		return;
	}

	ABaseCharacter* Character = Cast<ABaseCharacter>(GetOwner());
	UAnimInstance* AnimInstance = Character ? Character->GetMesh()->GetAnimInstance() : nullptr;
	if (!Character || !AnimInstance)
	{
		return;
	}

	// 不先 Stop 前一段：直接覆盖播放，敌人不会先回到 Idle。
	CurrentEnemyMove = nullptr;
	CurrentEnemyAction = nullptr;
	CurrentEnemyMontage = Montage;
	bEnemyComboWindowOpen = false;
	bActionInterrupted = false;
	bIsExecutionMontage = false;
	bIsGetupMontage = false;

	if (CombatContext)
	{
		CombatContext->SetPoiseState(0, 0, 0.f);
	}

	Character->PlayAnimMontage(Montage);
}

void URHEnemyCombatComponent::HandleEnemyActionMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage != CurrentEnemyMontage)
	{
		return;
	}

	const bool bWasExecutionMontage = bIsExecutionMontage;
	const bool bWasGetupMontage = bIsGetupMontage;
	bIsExecutionMontage = false;
	bIsGetupMontage = false;

	CurrentEnemyMontage = nullptr;
	CurrentEnemyMove = nullptr;
	CurrentEnemyAction = nullptr;
	bEnemyComboWindowOpen = false;
	CurrentCastPolarity = ERHOnomPolarity::None;
	DeactivateCastTrail();

	if (CombatContext)
	{
		CombatContext->SetPoiseState(0, 0, 0.f);
	}

	URHEnemyAIComponent* AI = GetOwner() ? GetOwner()->FindComponentByClass<URHEnemyAIComponent>() : nullptr;
	if (bWasExecutionMontage)
	{
		// 伤害已由处决蒙太奇里的 AN 结算；这里接 Dodge 的 Back 段，播完再回到状态树。
		UAnimMontage* DodgeBackMontage = AI ? AI->GetCurrentConfig().DodgeMontage.LoadSynchronous() : nullptr;
		if (DodgeBackMontage)
		{
			PlayDodgeBackMontage(DodgeBackMontage);
		}
		else if (AI)
		{
			AI->FinishExecution();
		}
	}
	else if (bWasGetupMontage)
	{
		if (AI && AI->IsBroken())
		{
			AI->FinishExecution();
		}
	}
}

void URHEnemyCombatComponent::HandleEnemyHitReceived(EKnsHitDirection Direction, EKnsHitReactionStrength Strength, int32 AttackPoiseLevel, int32 CurrentPoiseLevel)
{
	// 已死亡（本帧被打空血）：不再响应受击，避免 StopCurrentMoveMontage 打断死亡蒙太奇/重复发事件。
	if (IsOwnerDead())
	{
		return;
	}

	// 被打断：停掉当前敌人招式，让受击动画干净地播出来。
	bActionInterrupted = true;
	StopCurrentMoveMontage();

	// 受击落地：破防中再次受击 → 直接进入处刑（树已停，走组件层处决；原玩家按键/攻击输入处刑检测保留）。
	// 触发破防的那一下（EnterBreak 同帧）不处决，只进破防状态；跨帧的再次受击才处刑。
	if (URHEnemyAIComponent* AI = GetOwner() ? GetOwner()->FindComponentByClass<URHEnemyAIComponent>() : nullptr)
	{
		if (AI->IsBroken())
		{
			if (!AI->WasJustBroken())
			{
				AI->RequestExecutionFromHit();
			}
		}
		else if (!IsGuarding())
		{
			AI->SendAIEvent(FGameplayTag::RequestGameplayTag(TEXT("AI.Enemy.HitTaken"), false));
		}
	}
}

bool URHEnemyCombatComponent::IsClashableHitboxActive() const
{
	// 木桩判定框（ProbeBox）激活 = 命中框开启，可参与相杀；其它敌人仍按武器 hitbox 判定。
	if (const ARHTestDummy* Dummy = Cast<ARHTestDummy>(GetOwner()))
	{
		return Dummy->IsProbeEnabled();
	}
	return Super::IsClashableHitboxActive();
}

bool URHEnemyCombatComponent::ShouldSkipHitReaction() const
{
	// 破防（处决等待）中受击：不播受击动画（敌人已倒地，站立受击动画无意义），
	// 直接由 HandleEnemyHitReceived 触发处刑；PlayHitReaction 广播后 return。
	if (URHEnemyAIComponent* AI = GetOwner() ? GetOwner()->FindComponentByClass<URHEnemyAIComponent>() : nullptr)
	{
		if (AI->IsBroken())
		{
			return true;
		}
	}
	// 已死亡（本帧被打空血）：不播站立受击动画，死亡蒙太奇由 AI 组件接管播放。
	if (IsOwnerDead())
	{
		return true;
	}
	return Super::ShouldSkipHitReaction();
}

bool URHEnemyCombatComponent::IsOwnerDead() const
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return false;
	}
	if (UKnsAbilitySystemComponent* ASC = Owner->FindComponentByClass<UKnsAbilitySystemComponent>())
	{
		const FGameplayTag DeadTag = FGameplayTag::RequestGameplayTag(TEXT("Status.Dead"), false);
		return DeadTag.IsValid() && ASC->HasMatchingGameplayTag(DeadTag);
	}
	return false;
}

bool URHEnemyCombatComponent::ReportHit(AActor* TargetActor, FVector HitLocation, FGameplayTag HitboxTag)
{
	if (!ActiveHitboxTag.IsValid() || !ActiveHitboxTag.MatchesTagExact(HitboxTag))
	{
		return false;
	}

	if (!TargetActor || HitActorsThisHitbox.Contains(TargetActor))
	{
		return false;
	}

	if (!CurrentEnemyMove && !CurrentEnemyAction)
	{
		return false;
	}

	HitActorsThisHitbox.Add(TargetActor);

	FRHHitData HitData;
	HitData.Source = GetOwner();
	HitData.HitLocation = HitLocation;
	EKnsHitDirection ManualDirection;
	if (UKnsCombatComponent::ResolveHitDirectionTag(HitboxTag, ManualDirection))
	{
		HitData.bUseManualHitDirection = true;
		HitData.HitDirection = ManualDirection;
	}

	int32 HitStopLevel = 0;
	if (CurrentEnemyAction)
	{
		HitData.Damage = CurrentEnemyAction->ActionValue;
		HitData.PoiseLevel = CurrentEnemyAction->GetHitPoise();
		if (CurrentEnemyAction->AttackTag.IsValid())
		{
			HitData.Tags.AddTag(CurrentEnemyAction->AttackTag);
		}
		HitStopLevel = CurrentEnemyAction->HitStopLevel;
	}
	else
	{
		HitData.Damage = CurrentEnemyMove->ActionValue;
		HitData.PoiseLevel = CurrentEnemyMove->GetHitPoise();
		if (CurrentEnemyMove->AttackTag.IsValid())
		{
			HitData.Tags.AddTag(CurrentEnemyMove->AttackTag);
		}
		HitStopLevel = CurrentEnemyMove->HitStopLevel;
	}

	// 命中实际落地才播卡肉/震屏与命中广播（无敌/弹开等忽略命中的情况不触发）。
	if (!ApplyHitToTarget(TargetActor, HitData))
	{
		return true;
	}

	PlayHitStop(HitStopLevel);
	OnHitLanded.Broadcast(nullptr, TargetActor, HitLocation, HitboxTag);
	return true;
}

void URHEnemyCombatComponent::HandleCastEffect()
{
	if (!CurrentEnemyAction || !CurrentEnemyAction->EffectAbility)
	{
		return;
	}
	PlayEffectAbility(CurrentEnemyAction->EffectAbility);
}

void URHEnemyCombatComponent::PlayEffectAbility(TSubclassOf<UGameplayAbility> AbilityClass)
{
	if (!AbilityClass)
	{
		return;
	}

	if (UKnsAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		if (!ASC->FindAbilitySpecFromClass(AbilityClass))
		{
			ASC->GiveAbility(FGameplayAbilitySpec(AbilityClass, 1, INDEX_NONE, GetOwner()));
		}
		ASC->TryActivateAbilityByClass(AbilityClass, true);
	}
}
