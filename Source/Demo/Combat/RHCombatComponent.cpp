#include "RHCombatComponent.h"

#include "Animation/AnimInstance.h"
#include "Components/BoxComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Demo/Character/BaseCharacter.h"
#include "Demo/Combat/RHCombatActionInterface.h"
#include "Demo/Combat/RHHitData.h"
#include "Demo/Combat/KnsCombatContextComponent.h"
#include "Demo/Combat/Weapon/AWeaponBase.h"
#include "Demo/Component/RHEquipComponent.h"
#include "Demo/Component/KnsTargetLockComponent.h"
#include "Demo/GAS/KnsAbilitySystemComponent.h"
#include "Demo/GAS/KnsCommonAttributeSet.h"
#include "Demo/Onom/RHOnomComponent.h"
#include "Demo/Onom/RHOnomSettings.h"
#include "Demo/Onom/RHWeaponDefinition.h"
#include "GameFramework/Character.h"
#include "GameFramework/GameplayCameraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "TimerManager.h"

URHCombatComponent::URHCombatComponent()
{
	AttributeInitializer.bApplyOnBeginPlay = false;
}

void URHCombatComponent::BeginPlay()
{
	Super::BeginPlay();
	ApplyHealthInitializer();

	if (ABaseCharacter* Character = Cast<ABaseCharacter>(GetOwner()))
	{
		if (UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance())
		{
			AnimInstance->OnMontageEnded.AddDynamic(this, &URHCombatComponent::HandleActionMontageEnded);
		}
	}
}

void URHCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// 移动取消：Window.Cancel.Move 开启且有移动输入时，取消当前动作。
	if (ActionState == ERHActionState::Idle || !HasCancelTag(ERHCancelType::Move))
	{
		return;
	}

	if (ABaseCharacter* Character = Cast<ABaseCharacter>(GetOwner()))
	{
		if (!Character->GetMoveInputValue().IsNearlyZero())
		{
			CancelAction();
		}
	}
}

bool URHCombatComponent::ReportHit(AActor* TargetActor, FVector HitLocation, FGameplayTag HitboxTag)
{
	if (!ActiveHitboxTag.IsValid() || !ActiveHitboxTag.MatchesTagExact(HitboxTag))
	{
		return false;
	}

	if (!TargetActor || HitActorsThisHitbox.Contains(TargetActor))
	{
		return false;
	}

	const bool bIsActionHit = CurrentActionState.Action != nullptr;
	if (!CurrentMoveDefinition && !bIsActionHit)
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
	if (bIsActionHit)
	{
		HitData.Damage = CurrentActionState.Resolved.Damage;
		HitData.ResonanceDamage = CurrentActionState.Resolved.ResonanceDamage;
		HitData.CounterBarDamage = CurrentActionState.Resolved.CounterBarDamage;
		HitData.PoiseLevel = CurrentActionState.Resolved.HitPoise;
		if (CurrentActionState.Resolved.AttackTag.IsValid())
		{
			HitData.Tags.AddTag(CurrentActionState.Resolved.AttackTag);
		}
	}
	else
	{
		HitData.Damage = CurrentMoveDefinition->ActionValue;
		// 普通连段未配共振伤害时回落到武器默认值，避免“掉血但共振不涨”。
		const float MoveResonance = CurrentMoveDefinition->ResonanceDamage;
		HitData.ResonanceDamage = MoveResonance > 0.f
			? MoveResonance
			: (WeaponDefinition ? WeaponDefinition->DefaultResonanceDamage : 0.f);
		HitData.CounterBarDamage = CurrentMoveDefinition->CounterBarDamage;
		HitData.PoiseLevel = CurrentMoveDefinition->GetHitPoise();
		if (CurrentMoveDefinition->AttackTag.IsValid())
		{
			HitData.Tags.AddTag(CurrentMoveDefinition->AttackTag);
		}
	}

	// 命中实际落地（伤害/防御/破防结算）才播卡肉/震屏与命中广播；
	// 无敌帧（倒地/闪避）或敌人弹开等忽略命中的情况不触发。
	if (!ApplyHitToTarget(TargetActor, HitData))
	{
		return true;
	}

	PlayHitStop(bIsActionHit ? CurrentActionState.Resolved.HitStopLevel : CurrentMoveDefinition->HitStopLevel);
	OnRHActionLanded.Broadcast(bIsActionHit ? nullptr : CurrentMoveDefinition, TargetActor, HitLocation, HitboxTag);
	return true;
}

void URHCombatComponent::HandleAttack()
{
	if (IRHCombatActionInterface* ActionInterface = Cast<IRHCombatActionInterface>(GetOwner()))
	{
		if (ActionInterface->TryStartExecution())
		{
			return;
		}
	}

	if (IsIdle())
	{
		if (TryContinueComboAfterLoad())
		{
			return;
		}
		// 闪避派生攻击：持有 Action.Move.Dodge tag（翻滚中/翻滚与防御正常结束/防御主动退出）且段数为 0 时派生特殊招式。
		if (TryPlayDodgeAttack())
		{
			return;
		}
		// 跑步派生攻击：持有 Action.Move.Run 时优先播放跑步攻击招式。
		if (WeaponDefinition && WeaponDefinition->RunAttackMoveDefinition)
		{
			if (UKnsAbilitySystemComponent* ASC = GetAbilitySystemComponent())
			{
				const FGameplayTag RunTag = FGameplayTag::RequestGameplayTag(TEXT("Action.Move.Run"), false);
				if (RunTag.IsValid() && ASC->HasMatchingGameplayTag(RunTag))
				{
					PlayMove(WeaponDefinition->RunAttackMoveDefinition, ERHActionState::Attacking);
					return;
				}
			}
		}
		StartAttackChain();
		return;
	}

	if (bComboWindowOpen)
	{
		if (ActionState == ERHActionState::Attacking)
		{
			AdvanceAttack();
		}
		else
		{
			StartAttackChain();
		}
		return;
	}

	// 普攻取消：Window.Cancel.Attack 开启时普攻可打断当前动作（如战技）。
	if (HasCancelTag(ERHCancelType::Attack))
	{
		if (TryContinueComboAfterLoad())
		{
			return;
		}
		// 翻滚中按攻击（通过 Attack 取消窗口打断翻滚）：持有 Action.Move.Dodge tag 时直接派生闪避攻击。
		if (TryPlayDodgeAttack())
		{
			return;
		}
		StartAttackChain();
		return;
	}

	if (bPreInputWindowOpen)
	{
		StorePendingAttack();
	}
}

void URHCombatComponent::HandleSkill(int32 SkillIndex)
{
	if (!WeaponDefinition || !WeaponDefinition->Skills.IsValidIndex(SkillIndex))
	{
		return;
	}

	URHOnomActionDefinition* Skill = WeaponDefinition->Skills[SkillIndex];
	if (!Skill)
	{
		return;
	}

	if (IsIdle() || bComboWindowOpen || bChainWindowOpen || HasCancelTag(ERHCancelType::Special))
	{
		TryStartActionFlow(Skill);
		return;
	}

	if (bPreInputWindowOpen)
	{
		StorePendingSkill(SkillIndex);
	}
}

void URHCombatComponent::HandleRhythmWeapon(int32 WeaponIndex)
{
	URHEquipComponent* Equip = GetOwner()->FindComponentByClass<URHEquipComponent>();
	if (!Equip || !Equip->RhythmWeapons.IsValidIndex(WeaponIndex))
	{
		return;
	}

	URHOnomActionDefinition* Action = Equip->RhythmWeapons[WeaponIndex];
	if (!Action)
	{
		return;
	}

	if (IsIdle() || bComboWindowOpen || bChainWindowOpen || HasCancelTag(ERHCancelType::Special))
	{
		TryStartActionFlow(Action);
		return;
	}

	if (bPreInputWindowOpen)
	{
		StorePendingRhythmWeapon(Action);
	}
}

bool URHCombatComponent::TryStartActionFlow(URHOnomActionDefinition* Action)
{
	if (!Action)
	{
		return false;
	}

	URHOnomComponent* Onom = GetOnomComponent();

	// 派生战技：当前动作在链上且链窗口（RH Chain Window）开着时，
	// 按 NextActions 数组顺序检测，第一个消耗/蒙太奇都满足的动作立即进入，不再检测后面的；
	// 全部不符合则本次按键不生效（停留当前段，链不断）。
	if (bChainWindowOpen && CurrentActionState.Action && !CurrentActionState.Action->NextActions.IsEmpty())
	{
		for (URHOnomActionDefinition* NextAction : CurrentActionState.Action->NextActions)
		{
			if (!NextAction)
			{
				continue;
			}
			const FRHOnomConsumptionData NextPreview = Onom ? Onom->SimulateConsumeOnom(NextAction->RequiredCount) : FRHOnomConsumptionData();
			FRHOnomResolvedAction NextPreviewResolved;
			if (!NextAction->MatchesConsumption(NextPreview) || !NextAction->ResolveActionData(NextPreview, NextPreviewResolved))
			{
				continue; // 不符合 → 下一个
			}

			// 首个符合：实际消耗并开播下一段（不 CancelAction，链状态/trail 保留）。
			if (!TryStartAction(NextAction))
			{
				return false;
			}
			FRHOnomResolvedAction NextResolved;
			if (!NextAction->ResolveActionData(PendingActionCast.Consumption, NextResolved))
			{
				return false;
			}
			ClearComboBridge(); // 不残留普攻续段桥
			StartAction(NextAction, NextResolved);
			return true;
		}
		return false; // 遍历完没有符合的候选
	}

	// 用“实际会消耗的 RequiredCount 个”模拟数据匹配，保证预览极性=消耗后极性
	// （如蓝共鸣+三红消耗 2 → 共鸣(-1)+红(+1)=0 Neutral，而不是整手 +2 Major）。
	const FRHOnomConsumptionData Preview = Onom ? Onom->SimulateConsumeOnom(Action->RequiredCount) : FRHOnomConsumptionData();
	FRHOnomResolvedAction Resolved;
	FRHOnomResolvedAction PreviewResolved;
	if (!Action->MatchesConsumption(Preview) || !Action->ResolveActionData(Preview, PreviewResolved))
	{
		UE_LOG(LogTemp, Warning, TEXT("RH Action rejected: %s"), *Action->ActionId.ToString());
		return false;
	}

	if (!TryStartAction(Action))
	{
		return false;
	}

	// 变体/极性按“本次实际消耗的音形”解析，而不是消耗前的全部手牌预览
	// （例如 1 红 1 蓝消耗最外层红释放 → 大调，而不是整手算成平调）。
	if (!Action->ResolveActionData(PendingActionCast.Consumption, Resolved))
	{
		UE_LOG(LogTemp, Warning, TEXT("RH Action resolve failed after consume: %s"), *Action->ActionId.ToString());
		return false;
	}

	StartAction(Action, Resolved);
	return true;
}

void URHCombatComponent::HandleLoad()
{
	// load 本身作为 Special 类型取消：当前动作开了 Window.Cancel.Special 时可直接打断。
	if (!IsIdle() && !HasCancelTag(ERHCancelType::Special))
	{
		return;
	}

	URHOnomComponent* Onom = GetOnomComponent();
	if (!Onom)
	{
		return;
	}

	// 逆装填武器（bReload）：装填 = 共鸣槽 → 手牌，检测对象从 onom 换成共鸣槽。
	// 播装填动画（蒙太奇上放 RH Reload AN 触发 ExecuteReload）。
	if (WeaponDefinition && WeaponDefinition->bReload)
	{
		if (Onom->GetResonanceLayers() <= 0)
		{
			return; // 无共鸣槽：无法逆装填。
		}
		PlayLoadMontage();
		return;
	}

	if (Onom->HasGreyOnom())
	{
		// 有灰色音形：走抛弹（清空手牌含灰色 + 完美防御窗口）。
		PlayTossMontage();
	}
	else if (Onom->GetNonGreyOnomCount() > 0)
	{
		// 无灰色：走装填（非灰手牌全部存入共鸣）。
		PlayLoadMontage();
	}
}

void URHCombatComponent::ExecuteLoad()
{
	URHOnomComponent* Onom = GetOnomComponent();
	if (!Onom)
	{
		return;
	}

	if (Onom->HasGreyOnom())
	{
		// 有灰色：抛弹（清空手牌含灰色）。
		Onom->ClearHandOnom();
		UE_LOG(LogTemp, Warning, TEXT("RH Load Notify -> Toss: hand cleared"));
	}
	else
	{
		// 无灰色：装填（非灰手牌全部存入共鸣）。
		Onom->TryStoreToResonance();
	}
}

void URHCombatComponent::ExecuteReload(float ConsumeSeconds)
{
	URHOnomComponent* Onom = GetOnomComponent();
	if (!Onom)
	{
		return;
	}
	Onom->ReloadFromResonance(ConsumeSeconds);
}

void URHCombatComponent::HandleToss()
{
	// 已合并进 HandleLoad：有灰色走抛弹、无灰色走装填。
	HandleLoad();
}

void URHCombatComponent::PlayLoadMontage()
{
	if (!WeaponDefinition)
	{
		return;
	}
	const bool bFast = IsFastAllowed();
	UAnimMontage* Montage = bFast && WeaponDefinition->FastLoadMontage ? WeaponDefinition->FastLoadMontage.Get() : WeaponDefinition->LoadMontage.Get();
	ABaseCharacter* Character = Cast<ABaseCharacter>(GetOwner());
	if (!Montage || !Character)
	{
		return;
	}

	UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance();
	if (!AnimInstance)
	{
		return;
	}

	// 打断战技等动作进入装填时，丢弃未结算的动作上下文与施放反馈。
	if (CurrentActionMontage != nullptr)
	{
		ClearActionCastState();
	}

	CurrentActionMontage = Montage;
	CurrentMoveDefinition = nullptr;
	SetActionState(ERHActionState::Load);
	// 快速装填保留普攻连段（带超时），普通装填不保留。
	if (bFast)
	{
		StartComboBridge();
	}
	else
	{
		ClearComboBridge();
	}
	ClearCancelTags();
	if (CombatContext)
	{
		CombatContext->SetPoiseState(0, 0, 0.f);
	}
	Character->PlayAnimMontage(Montage);
}

void URHCombatComponent::PlayTossMontage()
{
	if (!WeaponDefinition)
	{
		return;
	}
	const bool bFast = IsFastAllowed();
	UAnimMontage* Montage = bFast && WeaponDefinition->FastTossMontage ? WeaponDefinition->FastTossMontage.Get() : WeaponDefinition->TossMontage.Get();
	ABaseCharacter* Character = Cast<ABaseCharacter>(GetOwner());
	if (!Montage || !Character)
	{
		return;
	}

	UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance();
	if (!AnimInstance)
	{
		return;
	}

	// 打断战技等动作进入抛弹时，丢弃未结算的动作上下文与施放反馈。
	if (CurrentActionMontage != nullptr)
	{
		ClearActionCastState();
	}

	CurrentActionMontage = Montage;
	CurrentMoveDefinition = nullptr;
	SetActionState(ERHActionState::Toss);
	ClearCancelTags();
	if (CombatContext)
	{
		CombatContext->SetPoiseState(0, 0, 0.f);
	}
	Character->PlayAnimMontage(Montage);
}

void URHCombatComponent::HandleDodge()
{
	if (!WeaponDefinition || !WeaponDefinition->DodgeMontage)
	{
		return;
	}

	if (!IsIdle() && !HasCancelTag(ERHCancelType::Roll))
	{
		return;
	}

	ABaseCharacter* Character = Cast<ABaseCharacter>(GetOwner());
	if (!Character)
	{
		return;
	}

	UAnimMontage* Montage = WeaponDefinition->DodgeMontage.Get();
	// 闪避前是否处于连段中（在播普攻或已有段数）：用于特定武器闪避后保留连段。
	const bool bComboActive = (ActionState == ERHActionState::Attacking) || (CurrentAttackIndex > 0);
	if (!IsIdle())
	{
		// 中断当前动作：丢弃未结算的战技上下文与施放反馈。
		CurrentActionState = FRHOnomActionState();
		PendingActionCast = FRHOnomActionCastContext();
		CurrentCastPolarity = ERHOnomPolarity::None;
	}

	CurrentActionMontage = Montage;
	CurrentMoveDefinition = nullptr;
	SetActionState(ERHActionState::Dodge);
	// 翻滚中：挂 Action.Move.Dodge tag（SetActionState 会先摘一次，这里挂回）——翻滚中按攻击取消即派生闪避攻击。
	SetDodgeDeriveTag(true);
	// 特定武器：闪避保留普攻连段桥（带超时；SetActionState 已清桥，这里重新开）。
	if (WeaponDefinition->bDodgePreservesCombo && bComboActive)
	{
		StartComboBridge();
	}
	ClearCancelTags();
	if (CombatContext)
	{
		CombatContext->SetPoiseState(0, 0, 0.f);
	}

	const FName Section = ResolveDodgeSection();
	Character->PlayAnimMontage(Montage, 1.f, Section);
}

void URHCombatComponent::HandleBlock(bool bBlock)
{
	if (bBlock)
	{
		// 已在格挡中：不重复播放格挡蒙太奇。
		if (bBlocking)
		{
			return;
		}

		if (!WeaponDefinition || !WeaponDefinition->DefensiveMontage)
		{
			return;
		}

		// 防御属于 Defensive 取消：当前动作开了 Window.Cancel.Defensive 时可直接打断。
		if (!IsIdle() && !HasCancelTag(ERHCancelType::Defensive))
		{
			return;
		}

		ABaseCharacter* Character = Cast<ABaseCharacter>(GetOwner());
		UAnimMontage* Montage = WeaponDefinition->DefensiveMontage.Get();
		if (!Character || !Montage)
		{
			return;
		}

		if (!IsIdle())
		{
			// 打断战技等动作进入防御：丢弃未结算的动作上下文与施放反馈。
			ClearActionCastState();
		}

		CurrentActionMontage = Montage;
		CurrentMoveDefinition = nullptr;
		SetActionState(ERHActionState::Block);
		ClearCancelTags();
		if (CombatContext)
		{
			CombatContext->SetPoiseState(0, 0, 0.f);
		}
		bBlocking = true;
		SetGuarding(true);
		Character->PlayAnimMontage(Montage);
		return;
	}

	ExitBlock();
}

void URHCombatComponent::ExitBlock()
{
	if (!bBlocking && ActionState != ERHActionState::Block)
	{
		return;
	}

	if (CurrentActionMontage)
	{
		if (ABaseCharacter* Character = Cast<ABaseCharacter>(GetOwner()))
		{
			if (UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance())
			{
				AnimInstance->Montage_Stop(0.1f, CurrentActionMontage);
			}
		}
	}

	ClearBlockState();
	CurrentActionMontage = nullptr;
	CurrentMoveDefinition = nullptr;
	SetActionState(ERHActionState::Idle);
	// 防御正常退出（非破防/非被打断）：挂闪避派生窗口（与翻滚/弹反结束后一致）。
	SetDodgeDeriveTag(true);
}

bool URHCombatComponent::IsBlocking() const
{
	return bBlocking;
}

void URHCombatComponent::ClearBlockState()
{
	if (bBlocking)
	{
		bBlocking = false;
		SetGuarding(false);
	}
}

void URHCombatComponent::HandleParry()
{
	if (!WeaponDefinition || !WeaponDefinition->DefensiveMontage)
	{
		return;
	}

	// 弹反属于 Defensive 取消：当前动作开了 Window.Cancel.Defensive 时可直接打断。
	if (!IsIdle() && !HasCancelTag(ERHCancelType::Defensive))
	{
		return;
	}

	ABaseCharacter* Character = Cast<ABaseCharacter>(GetOwner());
	UAnimMontage* Montage = WeaponDefinition->DefensiveMontage.Get();
	UAnimInstance* AnimInstance = Character ? Character->GetMesh()->GetAnimInstance() : nullptr;
	if (!Character || !Montage || !AnimInstance)
	{
		return;
	}

	// 弹反打断弹反（同一蒙太奇重播）：直接跳 section 重播，并先清掉取消 tag。
	// 不清 tag 的话旧播放开的取消窗口会残留，导致“刚重播完又能立刻被打断”的连按覆盖。
	if (CurrentActionMontage == Montage && AnimInstance->Montage_IsActive(Montage))
	{
		ClearCancelTags();
		const FName Section = bParryNextSectionIsR ? TEXT("R") : TEXT("L");
		bParryNextSectionIsR = !bParryNextSectionIsR;
		AnimInstance->Montage_JumpToSection(Section, Montage);
		return;
	}

	if (!IsIdle())
	{
		// 打断战技等动作进入弹反：丢弃未结算的动作上下文与施放反馈。
		ClearActionCastState();
	}

	CurrentActionMontage = Montage;
	CurrentMoveDefinition = nullptr;
	SetActionState(ERHActionState::Parry);
	ClearCancelTags();
	if (CombatContext)
	{
		CombatContext->SetPoiseState(0, 0, 0.f);
	}

	// 弹反动画 L/R 两个 section：无条件 flip-flop（每次弹反 L↔R 轮换，无需任何条件）。
	const FName Section = bParryNextSectionIsR ? TEXT("R") : TEXT("L");
	bParryNextSectionIsR = !bParryNextSectionIsR;
	Character->PlayAnimMontage(Montage, 1.f, Section);
}

void URHCombatComponent::HandleReReload()
{
	if (!WeaponDefinition || !WeaponDefinition->DefensiveMontage)
	{
		return;
	}

	// 逆转逆装填可随意使用（无需共鸣检测）：挂 tag 播蒙太奇；实际消耗与效果由蒙太奇内的 RH Reverse Just Reload Window 处理——
	// 窗口打开时消耗共鸣（无共鸣则窗口不生效），命中时按记录类型反色生成音形。

	// 与弹反同类：Defensive 取消 + 挂 Parry 状态/tag + 播 DefensiveMontage 的 ReReload 段。
	if (!IsIdle() && !HasCancelTag(ERHCancelType::Defensive))
	{
		return;
	}

	ABaseCharacter* Character = Cast<ABaseCharacter>(GetOwner());
	UAnimMontage* Montage = WeaponDefinition->DefensiveMontage.Get();
	UAnimInstance* AnimInstance = Character ? Character->GetMesh()->GetAnimInstance() : nullptr;
	if (!Character || !Montage || !AnimInstance)
	{
		return;
	}

	if (!IsIdle())
	{
		// 打断战技等动作进入 re-reload：丢弃未结算的动作上下文与施放反馈。
		ClearActionCastState();
	}

	CurrentActionMontage = Montage;
	CurrentMoveDefinition = nullptr;
	SetActionState(ERHActionState::Parry);
	ClearCancelTags();
	if (CombatContext)
	{
		CombatContext->SetPoiseState(0, 0, 0.f);
	}

	const FName Section = WeaponDefinition->ReReloadSectionName;
	Character->PlayAnimMontage(Montage, 1.f, Section);
}

void URHCombatComponent::HandleDefensiveInput(bool bPressed)
{
	if (!WeaponDefinition)
	{
		return;
	}
	switch (WeaponDefinition->DefensiveType)
	{
	case ERHDefensiveType::Parry:
		if (bPressed)
		{
			HandleParry();
		}
		break;
	case ERHDefensiveType::Defend:
		HandleBlock(bPressed);
		break;
	case ERHDefensiveType::ReverseReload:
		if (bPressed)
		{
			HandleReReload();
		}
		break;
	default:
		break;
	}
}

void URHCombatComponent::NotifyParrySuccess()
{
	// 弹反成功：不停止蒙太奇，打开除 Move 外全部取消类型——可接攻击/战技/装填/防御等，但不能用移动取消。
	PlayDefensiveCameraShake(ERHDefensiveShakeType::Parry);
	OpenCancel(ERHCancelType::Roll);
	OpenCancel(ERHCancelType::Attack);
	OpenCancel(ERHCancelType::Special);
	OpenCancel(ERHCancelType::Defensive);
}

void URHCombatComponent::SetClashWindow(bool bOpen)
{
	bClashWindowOpen = bOpen;
}

void URHCombatComponent::HandleWeaponClash(AActor* OtherWeapon, const FVector& HitLocation)
{
	if (!bClashWindowOpen || !OtherWeapon || !WeaponDefinition)
	{
		return;
	}
	if (HitActorsThisHitbox.Contains(OtherWeapon))
	{
		return; // 已与这把武器结算过。
	}
	HitActorsThisHitbox.Add(OtherWeapon);

	// 敌人身体也标记为本段已结算：先普通命中过就不许再相杀重复给 Onom。
	AActor* EnemyBody = OtherWeapon->GetOwner();
	if (!EnemyBody)
	{
		// 木桩等无武器实体：以自身为身体（HandleHitboxOverlap 对无 SpawnedWeapon 的敌人传 OtherActor 兜底）。
		EnemyBody = OtherWeapon;
	}
	if (!EnemyBody || HasResolvedHitboxTarget(EnemyBody))
	{
		return;
	}
	HitActorsThisHitbox.AddUnique(EnemyBody);

	// 对方命中框也完成结算：关闭对方当前命中框，避免它继续打到身体。
	if (UKnsCombatComponent* OtherCombat = EnemyBody->FindComponentByClass<UKnsCombatComponent>())
	{
		OtherCombat->ResolveWeaponClash(SpawnedWeapon);
	}

	// 本方命中框完成结算：关闭本方命中框。
	EndHitbox(ActiveHitboxTag);

	URHOnomComponent* Onom = GetOnomComponent();
	if (!Onom)
	{
		return;
	}

	// 相杀附加效果：手牌中所有灰色音形转换为 PerfectGuardHitRule（次音规则）的音形类型；无灰色则不动手牌。该效果独立于下方的满级共鸣奖励，生效与否都不影响三级共鸣。
	Onom->ConvertGreyOnom(WeaponDefinition->PerfectGuardHitRule.Type);

	// 相杀奖励：直接用 PerfectGuardHitRule 的类型获得满级共鸣（已有共鸣被覆盖）；反馈照旧固定 ClashSound + ClashVFX。
	Onom->SetResonance(URHOnomComponent::GetPolarityFromValue(WeaponDefinition->PerfectGuardHitRule.Type), Onom->GetMaxLayers());
	PlayClashFeedback(HitLocation);

	// 相杀成功：与弹反成功一致，打开除 Move 外全部取消类型——可接攻击/战技/装填/防御，不能用移动取消。
	OpenCancel(ERHCancelType::Roll);
	OpenCancel(ERHCancelType::Attack);
	OpenCancel(ERHCancelType::Special);
	OpenCancel(ERHCancelType::Defensive);
}

void URHCombatComponent::SetBlitzWindow(bool bOpen)
{
	if (bBlitzWindowOpen == bOpen)
	{
		return;
	}
	bBlitzWindowOpen = bOpen;

	ABaseCharacter* Character = Cast<ABaseCharacter>(GetOwner());
	if (!Character)
	{
		return;
	}
	USkeletalMeshComponent* Mesh = Character->GetMesh();
	if (!Mesh)
	{
		return;
	}

	if (bOpen)
	{
		// 开启期间骨骼网格体做查询重叠（只碰 Pawn/命中框通道，不碰胶囊体）。
		Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Mesh->SetGenerateOverlapEvents(true);
		Mesh->SetCollisionObjectType(ECC_GameTraceChannel1);
		Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
		Mesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
		Mesh->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Overlap);
		Mesh->OnComponentBeginOverlap.AddDynamic(this, &URHCombatComponent::HandleBlitzOverlap);
	}
	else
	{
		Mesh->OnComponentBeginOverlap.RemoveDynamic(this, &URHCombatComponent::HandleBlitzOverlap);
		Mesh->SetGenerateOverlapEvents(false);
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void URHCombatComponent::HandleBlitzOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!bBlitzWindowOpen || !OtherActor || OtherActor == GetOwner() || OtherActor == SpawnedWeapon)
	{
		return;
	}
	if (!Cast<ABaseCharacter>(OtherActor))
	{
		return;
	}
	if (HitActorsThisHitbox.Contains(OtherActor))
	{
		return;
	}

	FVector HitLocation = SweepResult.ImpactPoint;
	if (HitLocation.IsNearlyZero())
	{
		FVector ClosestPoint;
		if (OverlappedComponent && OverlappedComponent->GetClosestPointOnCollision(OtherActor->GetActorLocation(), ClosestPoint) >= 0.f)
		{
			HitLocation = ClosestPoint;
		}
		else
		{
			HitLocation = OtherActor->GetActorLocation();
		}
	}
	ReportHit(OtherActor, HitLocation, ActiveHitboxTag);
}

void URHCombatComponent::ExecuteBlitz(const FGameplayTag& HitboxTag, ERHOnomValue OnomValue)
{
	ABaseCharacter* Character = Cast<ABaseCharacter>(GetOwner());
	UWorld* World = GetWorld();
	if (!Character || !World)
	{
		return;
	}
	USkeletalMeshComponent* Mesh = Character->GetMesh();
	if (!Mesh)
	{
		return;
	}

	// 射线起点：玩家。
	const FVector Start = Character->GetActorLocation();

	// 终点：锁定目标位置；无锁定时 = 角色 forward × 1000。
	FVector End = Start + Character->GetActorForwardVector() * 1000.f;
	if (UKnsTargetLockComponent* Lock = Character->FindComponentByClass<UKnsTargetLockComponent>())
	{
		FVector TargetLocation;
		if (Lock->GetLockedTargetLocation(TargetLocation))
		{
			End = TargetLocation;
		}
	}

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Character);
	if (SpawnedWeapon)
	{
		Params.AddIgnoredActor(SpawnedWeapon);
	}

	FHitResult Hit;
	if (!World->LineTraceSingleByChannel(Hit, Start, End, ECC_Pawn, Params))
	{
		return;
	}
	AActor* HitActor = Hit.GetActor();
	if (!HitActor || HitActor == Character)
	{
		return;
	}

	// 伤害等数值从当前招式 DA 的解析结果获取（无招式时回落到武器默认，与 ReportHit 一致）。
	FRHHitData HitData;
	HitData.Source = Character;
	HitData.HitLocation = Hit.ImpactPoint;
	if (CurrentActionState.Action)
	{
		HitData.Damage = CurrentActionState.Resolved.Damage;
		HitData.ResonanceDamage = CurrentActionState.Resolved.ResonanceDamage;
		HitData.CounterBarDamage = CurrentActionState.Resolved.CounterBarDamage;
		HitData.PoiseLevel = CurrentActionState.Resolved.HitPoise;
		if (CurrentActionState.Resolved.AttackTag.IsValid())
		{
			HitData.Tags.AddTag(CurrentActionState.Resolved.AttackTag);
		}
	}
	else if (WeaponDefinition)
	{
		HitData.Damage = WeaponDefinition->DefaultDamage;
		HitData.ResonanceDamage = WeaponDefinition->DefaultResonanceDamage;
	}

	// 保留 HitboxTag 选择受击方向（HitReaction.Direction.{F,L,R,B}）。
	EKnsHitDirection ManualDirection;
	if (UKnsCombatComponent::ResolveHitDirectionTag(HitboxTag, ManualDirection))
	{
		HitData.bUseManualHitDirection = true;
		HitData.HitDirection = ManualDirection;
	}

	// Onom 极性选项：None=跟随武器 AttackHitRule；否则在武器规则基础上只替换音形类型。
	if (OnomValue != ERHOnomValue::None)
	{
		HitData.bOverrideOnomRule = true;
		HitData.OverrideOnomRule = WeaponDefinition ? WeaponDefinition->AttackHitRule : FRHOnomSourceRule();
		HitData.OverrideOnomRule.Type = OnomValue;
	}

	ApplyHitToTarget(HitActor, HitData);
	UE_LOG(LogTemp, Warning, TEXT("RH Blitz: ray hit %s (dir=%s onom=%s)"), *HitActor->GetName(), *HitboxTag.ToString(), *UEnum::GetValueAsString(OnomValue));
}

void URHCombatComponent::PlayClashFeedback(const FVector& HitLocation)
{
	if (!HitFeedbackDefinition)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	URHHitFeedbackDefinition* Def = HitFeedbackDefinition;
	if (Def->ClashSound)
	{
		UGameplayStatics::PlaySoundAtLocation(World, Def->ClashSound, HitLocation);
	}
	if (Def->ClashVFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, Def->ClashVFX, HitLocation);
	}
}

void URHCombatComponent::EnterHitReactionState()
{
	// 受击动画期间挂 Busy、无取消窗口：其它动作不能随意打断。
	// 要允许被打断，就在受击蒙太奇上手动放 RH Cancel AN 打开对应窗口。
	SetActionState(ERHActionState::HitReaction);
	ClearCancelTags();
}

void URHCombatComponent::ExitHitReactionState()
{
	if (ActionState == ERHActionState::HitReaction)
	{
		SetActionState(ERHActionState::Idle);
	}
}

void URHCombatComponent::TriggerDodgeTimeDilation(float TimeScale, float Duration)
{
	// 未显式传参时用组件上的可编辑属性。
	if (TimeScale <= 0.f)
	{
		TimeScale = DodgeTimeDilationScale;
	}
	if (Duration <= 0.f)
	{
		Duration = DodgeTimeDilationDuration;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	UGameplayStatics::SetGlobalTimeDilation(World, TimeScale);

	// 覆盖旧计时器，连续闪避时以最后一次为准。
	World->GetTimerManager().SetTimer(
		DodgeTimeDilationTimerHandle,
		FTimerDelegate::CreateWeakLambda(this, [World]()
		{
			if (World)
			{
				UGameplayStatics::SetGlobalTimeDilation(World, 1.f);
			}
		}),
		Duration,
		false);
}

void URHCombatComponent::HandleDodgeSuccess()
{
	// 共鸣时间奖励（基类：不加音效，音效挪到本组件的 DodgeFeedback）。
	Super::HandleDodgeSuccess();

	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (!Owner || !World)
	{
		return;
	}
	ABaseCharacter* Character = Cast<ABaseCharacter>(Owner);
	if (!Character)
	{
		return;
	}
	USkeletalMeshComponent* Mesh = Character->GetMesh();
	if (!Mesh)
	{
		return;
	}

	const float Duration = DodgeTimeDilationDuration;

	// 1. 闪避音效（原 RH Core Def 的 DodgeSFX，已挪到战斗组件）。
	if (DodgeFeedback.SFX)
	{
		UGameplayStatics::PlaySoundAtLocation(World, DodgeFeedback.SFX, Character->GetActorLocation());
	}

	// 2. mesh 覆盖材质：持续 Duration 后下掉（连续闪避时以最后一次为准，重复置空无害）。
	if (DodgeFeedback.OverlayMaterial)
	{
		Mesh->SetOverlayMaterial(DodgeFeedback.OverlayMaterial);
		TWeakObjectPtr<USkeletalMeshComponent> WeakMesh(Mesh);
		FTimerHandle OverlayTimer;
		World->GetTimerManager().SetTimer(
			OverlayTimer,
			[WeakMesh]()
			{
				if (USkeletalMeshComponent* ValidMesh = WeakMesh.Get())
				{
					ValidMesh->SetOverlayMaterial(nullptr);
				}
			},
			Duration,
			false);
	}

	// 3. 玩家 GameplayCamera 组件：激活 persistent visual rig，持续 Duration 后关闭。
	if (DodgeFeedback.CameraRig && Duration > 0.f)
	{
		if (UGameplayCameraComponent* GameplayCamera = Character->FindComponentByClass<UGameplayCameraComponent>())
		{
			const FCameraRigInstanceID RigID = GameplayCamera->ActivatePersistentVisualCameraRig(DodgeFeedback.CameraRig);
			TWeakObjectPtr<UGameplayCameraComponent> WeakCamera(GameplayCamera);
			FTimerHandle CameraRigTimer;
			World->GetTimerManager().SetTimer(
				CameraRigTimer,
				[WeakCamera, RigID]()
				{
					if (UGameplayCameraComponent* ValidCamera = WeakCamera.Get())
					{
						ValidCamera->DeactivateCameraRig(RigID);
					}
				},
				Duration,
				false);
		}
	}
}

void URHCombatComponent::ExecuteJustLoad(bool bPlayVFX)
{
	// 基类：执行 load 效果 + 音效（有灰→抛弹清手牌，无灰→装填入共鸣）。
	Super::ExecuteJustLoad(bPlayVFX);

	// bPlayVFX：在角色位置补播 parry 同款反馈（固定音效 + Neutral 档 VFX）。
	if (bPlayVFX)
	{
		const FVector Location = GetOwner() ? GetOwner()->GetActorLocation() : FVector::ZeroVector;
		PlayJustLoadFeedback(Location);
	}
}

void URHCombatComponent::TriggerParryEffect()
{
	// 弹反成功效果：不停止蒙太奇，打开除 Move 外全部取消类型 + 防御震屏。
	NotifyParrySuccess();
}

bool URHCombatComponent::GetHitOnomRuleOverride(FRHOnomSourceRule& OutRule) const
{
	if (CurrentMoveDefinition && CurrentMoveDefinition->bOverrideHitOnom)
	{
		OutRule = CurrentMoveDefinition->HitOnomRule;
		return true;
	}
	return false;
}

void URHCombatComponent::OpenJustLoadWindow()
{
	bJustLoadWindowActive = true;
}

void URHCombatComponent::CloseJustLoadWindow()
{
	bJustLoadWindowActive = false;
}

void URHCombatComponent::OpenJustReloadWindow(float ConsumeSeconds)
{
	bJustReloadWindowActive = true;
	JustReloadConsumeSeconds = ConsumeSeconds;
}

void URHCombatComponent::CloseJustReloadWindow()
{
	bJustReloadWindowActive = false;
}

void URHCombatComponent::OpenReverseJustReloadWindow(float ConsumeSeconds)
{
	URHOnomComponent* Onom = GetOnomComponent();
	// 窗口打开即消耗共鸣（沉没成本）：无共鸣/参数无效 → 窗口直接不生效，命中照常结算（不无敌）。
	if (!Onom || !Onom->PrepareReverseReload(ConsumeSeconds))
	{
		bReverseJustReloadWindowActive = false;
		return;
	}
	bReverseJustReloadWindowActive = true;
}

void URHCombatComponent::CloseReverseJustReloadWindow()
{
	bReverseJustReloadWindowActive = false;
}

bool URHCombatComponent::TryHandleJustWindowHit(const FRHHitData& HitData)
{
	URHOnomComponent* Onom = GetOnomComponent();
	if (!Onom)
	{
		return false;
	}

	// Just Load：无 onom（无非灰且无灰）时无法执行 load，窗口空转，命中照常结算（不无敌）。
	if (bJustLoadWindowActive)
	{
		if (Onom->GetNonGreyOnomCount() == 0 && !Onom->HasGreyOnom())
		{
			return false;
		}
		ExecuteJustLoad(false);
		PlayJustLoadFeedback(HitData.HitLocation);
		return true;
	}

	// Just Reload：无共鸣槽时无法执行 reload，不无敌。
	if (bJustReloadWindowActive)
	{
		if (Onom->GetResonanceLayers() <= 0)
		{
			return false;
		}
		Onom->ReloadFromResonance(JustReloadConsumeSeconds, false);
		PlayJustLoadFeedback(HitData.HitLocation);
		return true;
	}

	// Reverse Just Reload：共鸣已在窗口打开时消耗（沉没成本），命中时按预消耗记录的共鸣类型反色生成音形 + 命中无效化。
	if (bReverseJustReloadWindowActive)
	{
		Onom->DeliverReverseReload();
		PlayJustLoadFeedback(HitData.HitLocation);
		return true;
	}

	return false;
}

void URHCombatComponent::PlayJustLoadFeedback(const FVector& HitLocation)
{
	UWorld* World = GetWorld();
	if (!World || !HitFeedbackDefinition)
	{
		return;
	}

	// parry 同款：固定 ParrySound + ParryFeedback 按 Neutral 极性选特效。
	if (HitFeedbackDefinition->ParrySound)
	{
		UGameplayStatics::PlaySoundAtLocation(World, HitFeedbackDefinition->ParrySound, HitLocation);
	}
	FRHHitFeedbackPolarityEntry Entry;
	if (HitFeedbackDefinition->GetEntry(HitFeedbackDefinition->ParryFeedback, ERHOnomPolarity::Neutral, Entry) && Entry.VFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, Entry.VFX, HitLocation);
	}
}

FName URHCombatComponent::ResolveDodgeSection() const
{
	const ABaseCharacter* Character = Cast<ABaseCharacter>(GetOwner());
	if (!Character)
	{
		return TEXT("F");
	}

	const FVector2D MoveInput = Character->GetMoveInputValue();
	if (MoveInput.IsNearlyZero())
	{
		return TEXT("F");
	}

	const FVector Dir = Character->ConvertMoveInputToWorldDirection(MoveInput);
	if (Dir.IsNearlyZero())
	{
		return TEXT("F");
	}

	const FRotator ActorRotation = GetOwner() ? GetOwner()->GetActorRotation() : FRotator::ZeroRotator;
	const float DeltaYaw = FMath::FindDeltaAngleDegrees(ActorRotation.Yaw, Dir.Rotation().Yaw);
	const float Abs = FMath::Abs(DeltaYaw);
	if (Abs < 45.f)
	{
		return TEXT("F");
	}
	if (Abs > 135.f)
	{
		return TEXT("B");
	}
	// UE Yaw 正方向为顺时针（右），正差角 = 输入在右侧。
	return DeltaYaw > 0.f ? TEXT("R") : TEXT("L");
}

bool URHCombatComponent::IsFastAllowed() const
{
	if (UKnsAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		const FGameplayTag FastTag = FGameplayTag::RequestGameplayTag(TEXT("Window.AllowFast"), false);
		return FastTag.IsValid() && ASC->HasMatchingGameplayTag(FastTag);
	}
	return false;
}

bool URHCombatComponent::TryReleaseFinisher()
{
	URHOnomComponent* Onom = GetOnomComponent();
	if (!Onom || !WeaponDefinition || !WeaponDefinition->FinisherSkill)
	{
		return false;
	}

	if (Onom->GetChargePercent() < Onom->GetChargeMaxPercent())
	{
		return false;
	}

	if (!IsIdle() && !bComboWindowOpen)
	{
		return false;
	}

	URHOnomActionDefinition* Finisher = WeaponDefinition->FinisherSkill;
	FRHOnomResolvedAction Resolved;
	if (!Finisher->ResolveActionData(FRHOnomConsumptionData(), Resolved))
	{
		return false;
	}

	Onom->ResetCharge();
	StartAction(Finisher, Resolved);
	UE_LOG(LogTemp, Warning, TEXT("RH Finisher released: %s"), *Finisher->ActionId.ToString());
	return true;
}

bool URHCombatComponent::TryPlaySwitchTactic()
{
	if (!WeaponDefinition || !WeaponDefinition->SwitchTactic)
	{
		return false;
	}

	if (!IsIdle() && !bComboWindowOpen)
	{
		return false;
	}

	PlayMove(WeaponDefinition->SwitchTactic, ERHActionState::Attacking);
	UE_LOG(LogTemp, Warning, TEXT("RH Switch Tactic played: %s"), *WeaponDefinition->SwitchTactic->MoveId.ToString());
	return true;
}

void URHCombatComponent::HandleCastEffect()
{
	URHOnomActionDefinition* Action = CurrentActionState.Action;
	if (!Action)
	{
		return;
	}

	if (!Action->EffectAbilities.IsEmpty())
	{
		const int32 Index = CastEffectIndex % Action->EffectAbilities.Num();
		PlayEffectAbility(Action->EffectAbilities[Index]);
		++CastEffectIndex;
		return;
	}

	if (Action->EffectAbility)
	{
		PlayEffectAbility(Action->EffectAbility);
	}
}

bool URHCombatComponent::IsIdle() const
{
	return ActionState == ERHActionState::Idle;
}

ERHActionState URHCombatComponent::GetActionState() const
{
	return ActionState;
}

bool URHCombatComponent::CanConsumeOnom(int32 Amount) const
{
	if (IRHCombatActionInterface* ActionInterface = Cast<IRHCombatActionInterface>(GetOwner()))
	{
		return ActionInterface->CanConsumeOnom(Amount);
	}
	return false;
}

bool URHCombatComponent::TryConsumeOnom(int32 Amount)
{
	if (IRHCombatActionInterface* ActionInterface = Cast<IRHCombatActionInterface>(GetOwner()))
	{
		return ActionInterface->TryConsumeOnom(Amount);
	}
	return false;
}

void URHCombatComponent::SetComboWindowOpen(bool bOpen)
{
	bComboWindowOpen = bOpen;
	if (bOpen)
	{
		ConsumePendingAction();
	}
}

void URHCombatComponent::SetChainWindowOpen(bool bOpen)
{
	bChainWindowOpen = bOpen;
}

void URHCombatComponent::SetPreInputWindowOpen(bool bOpen)
{
	bPreInputWindowOpen = bOpen;
}

bool URHCombatComponent::HasPendingAction() const
{
	return bHasPendingAction;
}

namespace
{
	FName GetCancelTagName(ERHCancelType Type)
	{
		switch (Type)
		{
		case ERHCancelType::Roll: return TEXT("Window.Cancel.Roll");
		case ERHCancelType::Move: return TEXT("Window.Cancel.Move");
		case ERHCancelType::Attack: return TEXT("Window.Cancel.Attack");
		case ERHCancelType::Special: return TEXT("Window.Cancel.Special");
		case ERHCancelType::Defensive: return TEXT("Window.Cancel.Defensive");
		default: return NAME_None;
		}
	}

	FName GetPlayerStateTagName(ERHActionState State)
	{
		switch (State)
		{
		case ERHActionState::Attacking: return TEXT("State.Player.Attacking");
		case ERHActionState::Skill: return TEXT("State.Player.Skill");
		case ERHActionState::Load: return TEXT("State.Player.Load");
		case ERHActionState::Toss: return TEXT("State.Player.Toss");
		case ERHActionState::Dodge: return TEXT("State.Player.Dodge");
		case ERHActionState::Block: return TEXT("State.Player.Block");
		case ERHActionState::Parry: return TEXT("State.Player.Parry");
		default: return NAME_None;
		}
	}

}

bool URHCombatComponent::HasCancelTag(ERHCancelType Type) const
{
	UKnsAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC)
	{
		return false;
	}

	const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(GetCancelTagName(Type), false);
	return Tag.IsValid() && ASC->HasMatchingGameplayTag(Tag);
}

void URHCombatComponent::OpenCancel(ERHCancelType Type)
{
	UKnsAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}

	if (Type == ERHCancelType::Other)
	{
		// Other = 除当前动作类型外的全部取消类型（例如普攻蒙太奇上放 Other → 除普攻外都能打断）。
		ERHCancelType Excluded = ERHCancelType::Roll;
		bool bHasExcluded = true;
		switch (ActionState)
		{
		case ERHActionState::Attacking: Excluded = ERHCancelType::Attack; break;
		case ERHActionState::Skill:
		case ERHActionState::Load:
		case ERHActionState::Toss: Excluded = ERHCancelType::Special; break;
		case ERHActionState::Dodge: Excluded = ERHCancelType::Roll; break;
		case ERHActionState::Block:
		case ERHActionState::Parry: Excluded = ERHCancelType::Defensive; break;
		default: bHasExcluded = false; break; // Idle：没有“当前类型”，全部打开
		}

		const ERHCancelType Types[] = {
			ERHCancelType::Roll, ERHCancelType::Move, ERHCancelType::Attack,
			ERHCancelType::Special, ERHCancelType::Defensive
		};
		for (const ERHCancelType T : Types)
		{
			if (bHasExcluded && T == Excluded)
			{
				continue;
			}
			const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(GetCancelTagName(T), false);
			if (Tag.IsValid())
			{
				ASC->AddLooseGameplayTag(Tag);
				if (T == ERHCancelType::Move && CombatContext)
				{
					// 上 Move 取消 tag 时韧性归零（期间可被任意打断）。
					CombatContext->SetPoiseState(0, 0, 0.f);
				}
			}
		}
		return;
	}

	const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(GetCancelTagName(Type), false);
	if (Tag.IsValid())
	{
		ASC->AddLooseGameplayTag(Tag);
		if (Type == ERHCancelType::Move && CombatContext)
		{
			// 上 Move 取消 tag 时韧性归零（期间可被任意打断）。
			CombatContext->SetPoiseState(0, 0, 0.f);
		}
	}
}

void URHCombatComponent::ClearCancelTags()
{
	UKnsAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}

	const ERHCancelType Types[] = { ERHCancelType::Roll, ERHCancelType::Move, ERHCancelType::Attack, ERHCancelType::Special, ERHCancelType::Defensive };
	for (const ERHCancelType Type : Types)
	{
		const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(GetCancelTagName(Type), false);
		if (Tag.IsValid())
		{
			ASC->RemoveLooseGameplayTag(Tag);
		}
	}
}

void URHCombatComponent::CancelAction()
{
	if (IsIdle())
	{
		return;
	}

	ClearBlockState();
	ClearComboBridge();
	SetDodgeDeriveTag(false);
	CurrentChainDepth = 0;
	// 取消动作：关闭武器拖尾（与正常结束一致）。
	DeactivateCastTrail();

	if (CurrentActionMontage)
	{
		if (ABaseCharacter* Character = Cast<ABaseCharacter>(GetOwner()))
		{
			if (UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance())
			{
				AnimInstance->Montage_Stop(0.1f, CurrentActionMontage);
			}
		}
	}

	CurrentActionMontage = nullptr;
	CurrentMoveDefinition = nullptr;
	CurrentActionState = FRHOnomActionState();
	CurrentAttackIndex = 0;
	SetActionState(ERHActionState::Idle);
}

void URHCombatComponent::InitializeAfterAbilitySystem()
{
	ApplyHealthInitializer();
}

FRHOnomActionContext URHCombatComponent::GetLastActionContext() const
{
	return LastActionContext;
}

void URHCombatComponent::HandleActionMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage != CurrentActionMontage)
	{
		return;
	}

	// 翻滚/防御性动作（弹反/逆转逆装填等）蒙太奇结束：正常播完 → 挂闪避派生窗口；被打断 → 窗口作废。
	const bool bIsDodgeOrDefensiveMontage = WeaponDefinition
		&& (Montage == WeaponDefinition->DodgeMontage.Get() || Montage == WeaponDefinition->DefensiveMontage.Get());

	ClearBlockState();
	CurrentActionMontage = nullptr;
	CurrentMoveDefinition = nullptr;
	CurrentActionState = FRHOnomActionState();
	CurrentChainDepth = 0;
	if (!bPreserveCombo)
	{
		CurrentAttackIndex = 0;
	}
	// 动作释放完（正常结束/被打断）：关闭武器拖尾。
	DeactivateCastTrail();
	SetActionState(ERHActionState::Idle);
	// 动作结束回 Idle：桥若还开着，重新起超时——从玩家可操作时刻（而非开桥时刻）起算。
	if (bPreserveCombo && WeaponDefinition)
	{
		StartComboBridge();
	}
	if (bIsDodgeOrDefensiveMontage)
	{
		SetDodgeDeriveTag(!bInterrupted);
	}
}

bool URHCombatComponent::TryContinueComboAfterLoad()
{
	if (!bPreserveCombo || !WeaponDefinition)
	{
		return false;
	}

	const int32 NextIndex = CurrentAttackIndex + 1;
	if (!WeaponDefinition->Attacks.IsValidIndex(NextIndex))
	{
		return false; // 连段到头：回落 StartAttackChain 开新连段。
	}

	// 消费桥（并取消超时，避免超时回调误清新连段段数）。
	ClearComboBridge();
	CurrentAttackIndex = NextIndex;
	PlayMove(WeaponDefinition->Attacks[NextIndex], ERHActionState::Attacking);
	return true;
}

void URHCombatComponent::StartComboBridge()
{
	bPreserveCombo = true;
	UWorld* World = GetWorld();
	if (!World || !WeaponDefinition)
	{
		return;
	}
	const float Timeout = WeaponDefinition->ComboBridgeTimeout;
	if (Timeout > 0.f)
	{
		// 覆盖旧计时器：连续开桥以最后一次为准。
		World->GetTimerManager().SetTimer(
			ComboBridgeTimerHandle,
			this,
			&URHCombatComponent::HandleComboBridgeTimeout,
			Timeout,
			false);
	}
}

void URHCombatComponent::ClearComboBridge()
{
	bPreserveCombo = false;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ComboBridgeTimerHandle);
	}
}

void URHCombatComponent::HandleComboBridgeTimeout()
{
	// 超时未续段：桥作废、段数归零（下次普攻从头开始）。
	bPreserveCombo = false;
	CurrentAttackIndex = 0;
}

void URHCombatComponent::StartAttackChain()
{
	if (!WeaponDefinition || WeaponDefinition->Attacks.IsEmpty())
	{
		return;
	}

	CurrentAttackIndex = 0;
	PlayMove(WeaponDefinition->Attacks[0], ERHActionState::Attacking);
}

void URHCombatComponent::SetDodgeDeriveTag(bool bEnabled)
{
	UKnsAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}
	const FGameplayTag DodgeTag = FGameplayTag::RequestGameplayTag(TEXT("Action.Move.Dodge"), false);
	if (!DodgeTag.IsValid())
	{
		return;
	}
	if (bEnabled)
	{
		ASC->AddLooseGameplayTag(DodgeTag);
	}
	else
	{
		ASC->RemoveLooseGameplayTag(DodgeTag);
	}
}

bool URHCombatComponent::TryPlayDodgeAttack()
{
	if (!WeaponDefinition || !WeaponDefinition->DodgeAttackMoveDefinition || CurrentAttackIndex != 0)
	{
		return false;
	}
	UKnsAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC)
	{
		return false;
	}
	const FGameplayTag DodgeTag = FGameplayTag::RequestGameplayTag(TEXT("Action.Move.Dodge"), false);
	if (!DodgeTag.IsValid() || !ASC->HasMatchingGameplayTag(DodgeTag))
	{
		return false;
	}
	// 消费窗口（PlayMove → SetActionState 也会摘，幂等），然后播闪避派生招式。
	SetDodgeDeriveTag(false);
	PlayMove(WeaponDefinition->DodgeAttackMoveDefinition, ERHActionState::Attacking);
	return true;
}

void URHCombatComponent::AdvanceAttack()
{
	if (!WeaponDefinition)
	{
		return;
	}

	const int32 NextIndex = CurrentAttackIndex + 1;
	if (!WeaponDefinition->Attacks.IsValidIndex(NextIndex))
	{
		return;
	}

	CurrentAttackIndex = NextIndex;
	PlayMove(WeaponDefinition->Attacks[NextIndex], ERHActionState::Attacking);
}

void URHCombatComponent::PlayMove(URHMoveDefinition* Move, ERHActionState NewState)
{
	if (!Move)
	{
		return;
	}

	UAnimMontage* NewMontage = Move->Montage.LoadSynchronous();
	if (!NewMontage)
	{
		return;
	}

	ABaseCharacter* Character = Cast<ABaseCharacter>(GetOwner());
	if (!Character)
	{
		return;
	}

	UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance();
	if (!AnimInstance)
	{
		return;
	}

	UAnimMontage* OldMontage = CurrentActionMontage;
	const bool bSameMontage = (OldMontage == NewMontage);

	if (OldMontage != nullptr)
	{
		// 从其它动作（战技/装填等）打断切入：丢弃未结算的动作上下文与施放反馈，
		// 否则后续普攻会被当成战技命中结算（伤害/增幅/音形规则错误），武器覆层也不会清除。
		ClearActionCastState();
	}

	CurrentMoveDefinition = Move;
	CurrentActionMontage = NewMontage;
	SetActionState(NewState);
	ClearCancelTags();
	if (CombatContext)
	{
		CombatContext->SetPoiseState(0, Move->PoiseLevel, Move->Resistance);
	}
	if (UKnsAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		if (!ActiveMoveGrantedTags.IsEmpty())
		{
			ASC->RemoveLooseGameplayTags(ActiveMoveGrantedTags);
		}
		ActiveMoveGrantedTags = Move->GrantedTags;
		ActiveMoveGrantedTags.AppendTags(Move->AIIntentTags);
		ASC->AddLooseGameplayTags(ActiveMoveGrantedTags);
	}

	if (bSameMontage)
	{
		// 同一蒙太奇连段：直接跳 Section，避免回到 idle 再播放。
		if (Move->SectionName != NAME_None)
		{
			if (AnimInstance->Montage_IsActive(NewMontage))
			{
				AnimInstance->Montage_JumpToSection(Move->SectionName, NewMontage);
			}
			else
			{
				AnimInstance->Montage_Play(NewMontage, Move->PlayRate);
				AnimInstance->Montage_JumpToSection(Move->SectionName, NewMontage);
			}
		}
		else
		{
			AnimInstance->Montage_Play(NewMontage, Move->PlayRate);
		}
		return;
	}

	// 与 MH Combo 一致：直接 PlayAnimMontage(含起始 Section)，由蒙太奇系统打断旧动画，顺畅衔接。
	Character->PlayAnimMontage(NewMontage, Move->PlayRate, Move->SectionName);
}

void URHCombatComponent::ClearActionCastState()
{
	CurrentActionState = FRHOnomActionState();
	PendingActionCast = FRHOnomActionCastContext();
	CurrentCastPolarity = ERHOnomPolarity::None;
	RestoreTemporaryWeapon();
}

void URHCombatComponent::EnterTemporaryWeapon(URHWeaponDefinition* InWeaponDefinition)
{
	RestoreTemporaryWeapon();
	// 临时武器的武器类/主手 Socket 从 DA 取。
	if (!InWeaponDefinition || !InWeaponDefinition->WeaponClass)
	{
		return;
	}

	ABaseCharacter* Character = Cast<ABaseCharacter>(GetOwner());
	UWorld* World = GetWorld();
	if (!Character || !World || !Character->GetMesh())
	{
		return;
	}

	OriginalTemporaryWeapon = SpawnedWeapon;
	if (SpawnedWeapon)
	{
		SpawnedWeapon->SetActorHiddenInGame(true);
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = Character;
	SpawnParams.Instigator = Character->GetInstigator();
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	TemporaryWeaponActor = World->SpawnActor<AWeaponBase>(InWeaponDefinition->WeaponClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
	if (TemporaryWeaponActor)
	{
		TemporaryWeaponActor->AttachToComponent(
			Character->GetMesh(),
			FAttachmentTransformRules::SnapToTargetNotIncludingScale,
			InWeaponDefinition->WeaponSocketName);
		bTemporaryWeaponActive = true;
	}
	else
	{
		if (OriginalTemporaryWeapon)
		{
			OriginalTemporaryWeapon->SetActorHiddenInGame(false);
		}
		OriginalTemporaryWeapon = nullptr;
	}
}

void URHCombatComponent::RestoreTemporaryWeapon()
{
	if (!bTemporaryWeaponActive)
	{
		return;
	}

	if (TemporaryWeaponActor)
	{
		TemporaryWeaponActor->Destroy();
		TemporaryWeaponActor = nullptr;
	}
	if (OriginalTemporaryWeapon)
	{
		OriginalTemporaryWeapon->SetActorHiddenInGame(false);
	}
	OriginalTemporaryWeapon = nullptr;
	bTemporaryWeaponActive = false;
}

void URHCombatComponent::StartAction(URHOnomActionDefinition* Action, const FRHOnomResolvedAction& Resolved)
{
	if (!Action)
	{
		return;
	}

	UAnimMontage* NewMontage = Resolved.Montage.LoadSynchronous();
	if (!NewMontage)
	{
		return;
	}

	ABaseCharacter* Character = Cast<ABaseCharacter>(GetOwner());
	if (!Character)
	{
		return;
	}

	CurrentActionState.Action = Action;
	CurrentActionState.Resolved = Resolved;
	CurrentActionState.ConsumedCount = PendingActionCast.Consumption.ConsumedCount;
	CurrentActionState.ConsumedAbsoluteSum = PendingActionCast.Consumption.AbsoluteSum;
	// 派生链深度：链窗口开着 = 从上一段派生过来 → 深度 +1；否则新起手/非链动作 → 归 0。
	// 只判断窗口而非 NextActions 是否非空，保证派生到链终点段时深度不误清。
	CurrentChainDepth = bChainWindowOpen ? CurrentChainDepth + 1 : 0;
	CurrentMoveDefinition = nullptr;
	CurrentActionMontage = NewMontage;
	SetActionState(ERHActionState::Skill);
	ClearCancelTags();

	if (CombatContext)
	{
		CombatContext->SetPoiseState(0, Resolved.PoiseLevel, Resolved.Resistance);
	}

	LastActionContext.Source = GetOwner();
	LastActionContext.ActionDefinition = Action;
	LastActionContext.Consumption = PendingActionCast.Consumption;
	LastActionContext.DiscountMultiplier = PendingActionCast.DiscountMultiplier;
	LastActionContext.Damage = Resolved.Damage;
	LastActionContext.ResonanceDamage = Resolved.ResonanceDamage;

	// 施放极性（由消耗和值决定；无消耗动作 = None，不做施放反馈）。
	ERHOnomPolarity Polarity = ERHOnomPolarity::None;
	const FRHOnomConsumptionData& Consumption = PendingActionCast.Consumption;
	if (Consumption.ConsumedCount > 0)
	{
		Polarity = Consumption.SignedSum > 0
			? ERHOnomPolarity::Major
			: (Consumption.SignedSum < 0 ? ERHOnomPolarity::Minor : ERHOnomPolarity::Neutral);
	}
	LastActionContext.Polarity = Polarity;
	CurrentCastPolarity = Polarity;
	CastEffectIndex = 0;

	if (UKnsAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		if (!ActiveMoveGrantedTags.IsEmpty())
		{
			ASC->RemoveLooseGameplayTags(ActiveMoveGrantedTags);
		}
		ActiveMoveGrantedTags = Resolved.GrantedTags;
		ActiveMoveGrantedTags.AppendTags(Action->AIIntentTags);
		ASC->AddLooseGameplayTags(ActiveMoveGrantedTags);
	}

	// 释放 action：自动激活武器拖尾（战技/音律武器/终结技统一入口，释放完/取消/被打断时统一关闭）。
	ActivateCastTrail();

	Character->PlayAnimMontage(NewMontage, Resolved.PlayRate, Resolved.SectionName);
}

void URHCombatComponent::PlayEffectAbility(TSubclassOf<UGameplayAbility> AbilityClass)
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

void URHCombatComponent::StorePendingAttack()
{
	bHasPendingAction = true;
	PendingActionState = ERHActionState::Attacking;
	PendingAttackIndex = ActionState == ERHActionState::Attacking ? CurrentAttackIndex + 1 : 0;
	PendingSkillIndex = INDEX_NONE;
	PendingRhythmWeapon = nullptr;
}

void URHCombatComponent::StorePendingSkill(int32 SkillIndex)
{
	bHasPendingAction = true;
	PendingActionState = ERHActionState::Skill;
	PendingSkillIndex = SkillIndex;
	PendingRhythmWeapon = nullptr;
}

void URHCombatComponent::StorePendingRhythmWeapon(URHOnomActionDefinition* Action)
{
	bHasPendingAction = true;
	PendingActionState = ERHActionState::Skill;
	PendingRhythmWeapon = Action;
	PendingSkillIndex = INDEX_NONE;
}

void URHCombatComponent::ConsumePendingAction()
{
	if (!bHasPendingAction)
	{
		return;
	}

	if (PendingActionState == ERHActionState::Attacking && WeaponDefinition && WeaponDefinition->Attacks.IsValidIndex(PendingAttackIndex))
	{
		CurrentAttackIndex = PendingAttackIndex;
		PlayMove(WeaponDefinition->Attacks[PendingAttackIndex], ERHActionState::Attacking);
	}
	else if (PendingActionState == ERHActionState::Skill)
	{
		URHOnomActionDefinition* Action = nullptr;
		if (PendingSkillIndex != INDEX_NONE && WeaponDefinition && WeaponDefinition->Skills.IsValidIndex(PendingSkillIndex))
		{
			Action = WeaponDefinition->Skills[PendingSkillIndex];
		}
		else if (PendingRhythmWeapon)
		{
			Action = PendingRhythmWeapon;
		}

		if (Action)
		{
			TryStartActionFlow(Action);
		}
	}

	bHasPendingAction = false;
	PendingActionState = ERHActionState::Idle;
	PendingSkillIndex = INDEX_NONE;
	PendingRhythmWeapon = nullptr;
}

void URHCombatComponent::SetActionState(ERHActionState NewState)
{
	ActionState = NewState;

	if (NewState == ERHActionState::Idle)
	{
		RestoreTemporaryWeapon();
	}

	UKnsAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}

	// State.Player.Busy = 任意动作的通用忙 tag；State.Player.<行为> = 每个行为单独一个。
	const FGameplayTag BusyTag = FGameplayTag::RequestGameplayTag(TEXT("State.Player.Busy"), false);
	const FName StateTagName = GetPlayerStateTagName(NewState);
	const FGameplayTag StateTag = StateTagName.IsNone() ? FGameplayTag() : FGameplayTag::RequestGameplayTag(StateTagName, false);
	const FName AllStateTagNames[] = {
		TEXT("State.Player.Attacking"), TEXT("State.Player.Skill"), TEXT("State.Player.Load"),
		TEXT("State.Player.Toss"), TEXT("State.Player.Dodge"), TEXT("State.Player.Block"), TEXT("State.Player.Parry")
	};

	if (NewState == ERHActionState::Idle)
	{
		// 摘下通用忙 + 所有行为 tag。
		if (BusyTag.IsValid())
		{
			ASC->RemoveLooseGameplayTag(BusyTag);
		}
		for (const FName TagName : AllStateTagNames)
		{
			const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(TagName, false);
			if (Tag.IsValid())
			{
				ASC->RemoveLooseGameplayTag(Tag);
			}
		}
		bHasPendingAction = false;
		PendingActionState = ERHActionState::Idle;
		PendingAttackIndex = 0;
		PendingSkillIndex = INDEX_NONE;
		PendingRhythmWeapon = nullptr;
		CurrentActionState = FRHOnomActionState();
		PendingActionCast = FRHOnomActionCastContext();
		if (CombatContext)
		{
			CombatContext->ClearCombatState();
		}
		ClearCancelTags();
		CurrentCastPolarity = ERHOnomPolarity::None;
	}
	else
	{
		// 任何动作起手：闪避派生窗口作废（Action.Move.Dodge tag 由战斗组件统一管理，只有闪避/防御后的下一次普攻能派生）。
		SetDodgeDeriveTag(false);
		if (NewState != ERHActionState::Load)
		{
			// 只有装填动作能延续连段桥；其它动作起手即断（并取消超时）。
			ClearComboBridge();
		}
		if (BusyTag.IsValid() && !ASC->HasMatchingGameplayTag(BusyTag))
		{
			ASC->AddLooseGameplayTag(BusyTag);
		}
		// 摘下旧行为 tag，再挂当前行为 tag（切换行为不残留上一个的）。
		for (const FName TagName : AllStateTagNames)
		{
			const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(TagName, false);
			if (Tag.IsValid())
			{
				ASC->RemoveLooseGameplayTag(Tag);
			}
		}
		if (StateTag.IsValid())
		{
			ASC->AddLooseGameplayTag(StateTag);
		}
	}

	if (NewState == ERHActionState::Idle && !ActiveMoveGrantedTags.IsEmpty())
	{
		ASC->RemoveLooseGameplayTags(ActiveMoveGrantedTags);
		ActiveMoveGrantedTags.Reset();
	}
}

void URHCombatComponent::ApplyHealthInitializer()
{
	UKnsAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}

	if (UKnsCommonAttributeSet* CommonAS = const_cast<UKnsCommonAttributeSet*>(ASC->GetSet<UKnsCommonAttributeSet>()))
	{
		CommonAS->InitMaxHealth(MaxHealth);
		CommonAS->InitHealth(FMath::Clamp(Health, 0.f, MaxHealth));
	}
}
