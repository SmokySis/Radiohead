#include "KnsAbilitySystemComponent.h"

#include "AbilitySystemInterface.h"
#include "Demo/AI/RHEnemyAIComponent.h"
#include "Demo/Character/BaseCharacter.h"
#include "Demo/Combat/KnsCombatComponent.h"
#include "Demo/Combat/RHCombatComponent.h"
#include "Demo/Combat/RHHitFeedbackDefinition.h"
#include "Demo/Onom/RHWeaponDefinition.h"
#include "GameplayTagContainer.h"
#include "Kismet/GameplayStatics.h"
#include "KnsCommonAttributeSet.h"
#include "KnsDamageGameplayEffect.h"
#include "KnsPlayerAttributeSet.h"
#include "KnsResourceGameplayEffect.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Demo/Onom/RHOnomComponent.h"

namespace
{
	void PlayHitFeedback(UKnsCombatComponent* AttackerCombat, bool bGuarded, const FRHHitData& HitData, AActor* Target)
	{
		if (!AttackerCombat || !AttackerCombat->HitFeedbackDefinition)
		{
			return;
		}

		URHHitFeedbackDefinition* Def = AttackerCombat->HitFeedbackDefinition;
		UWorld* World = Target ? Target->GetWorld() : nullptr;
		if (!World)
		{
			return;
		}

		USoundBase* Sound = nullptr;
		UNiagaraSystem* VFX = nullptr;

		if (bGuarded)
		{
			Sound = Def->GuardSound;
			VFX = Def->GuardVFX;
		}
		else
		{
			// 普攻按"会获得的 Onom 极性"，战技按"消耗极性"选档；命中数据显式指定规则（Blitz 选极性）时按该极性。
			ERHOnomPolarity Polarity = (HitData.bIsSkill && !HitData.bOverrideOnomRule)
				? AttackerCombat->GetCurrentCastPolarity()
				: AttackerCombat->GetCurrentHitOnomPolarity();
			// 终结技等消耗充能而非 Onom 的招式没有施放极性（None），命中特效回落 Neutral 档。
			if (Polarity == ERHOnomPolarity::None)
			{
				Polarity = ERHOnomPolarity::Neutral;
			}
			const TArray<FRHHitFeedbackPolarityEntry>& Entries = (HitData.bIsSkill && !HitData.bOverrideOnomRule) ? Def->SkillHitFeedback : Def->AttackHitFeedback;
			FRHHitFeedbackPolarityEntry Entry;
			if (Def->GetEntry(Entries, Polarity, Entry))
			{
				Sound = Entry.Sound;
				VFX = Entry.VFX;
			}
		}

		if (Sound)
		{
			UGameplayStatics::PlaySoundAtLocation(World, Sound, HitData.HitLocation);
		}
		if (VFX)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, VFX, HitData.HitLocation);
		}
	}

	void PlayParryFeedback(UKnsCombatComponent* DefenderCombat, ERHOnomPolarity Polarity, const FRHHitData& HitData)
	{
		if (!DefenderCombat || !DefenderCombat->HitFeedbackDefinition)
		{
			return;
		}

		URHHitFeedbackDefinition* Def = DefenderCombat->HitFeedbackDefinition;
		UWorld* World = DefenderCombat->GetWorld();
		if (!World)
		{
			return;
		}

		// 完美防御：固定音效 + 按本次获得的 Onom 极性选特效。
		if (Def->ParrySound)
		{
			UGameplayStatics::PlaySoundAtLocation(World, Def->ParrySound, HitData.HitLocation);
		}
		FRHHitFeedbackPolarityEntry Entry;
		if (Def->GetEntry(Def->ParryFeedback, Polarity, Entry) && Entry.VFX)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, Entry.VFX, HitData.HitLocation);
		}
	}
}

UKnsAbilitySystemComponent::UKnsAbilitySystemComponent()
{
}

float UKnsAbilitySystemComponent::GetAttributeValue(const FGameplayAttribute& Attribute) const
{
	return GetNumericAttribute(Attribute);
}

float UKnsAbilitySystemComponent::GetStamina() const
{
	return GetAttributeValue(UKnsCommonAttributeSet::GetStaminaAttribute());
}

float UKnsAbilitySystemComponent::GetMaxStamina() const
{
	return GetAttributeValue(UKnsCommonAttributeSet::GetMaxStaminaAttribute());
}

bool UKnsAbilitySystemComponent::HasEnoughStamina(float Amount) const
{
	return Amount <= 0.f || GetStamina() >= Amount;
}

bool UKnsAbilitySystemComponent::TryConsumeStamina(float Amount)
{
	if (!HasEnoughStamina(Amount))
	{
		return false;
	}

	ApplyResourceChange(TEXT("Resource.Stamina"), -Amount);
	return true;
}

void UKnsAbilitySystemComponent::RegenerateStamina(float Amount)
{
	if (Amount <= 0.f)
	{
		return;
	}

	const float Missing = FMath::Max(0.f, GetMaxStamina() - GetStamina());
	if (Missing > 0.f)
	{
		ApplyResourceChange(TEXT("Resource.Stamina"), FMath::Min(Amount, Missing));
	}
}

float UKnsAbilitySystemComponent::GetOnom() const
{
	return GetAttributeValue(UKnsPlayerAttributeSet::GetOnomAttribute());
}

float UKnsAbilitySystemComponent::GetMaxOnom() const
{
	return GetAttributeValue(UKnsPlayerAttributeSet::GetMaxOnomAttribute());
}

bool UKnsAbilitySystemComponent::HasEnoughOnom(float Amount) const
{
	return Amount <= 0.f || GetOnom() >= Amount;
}

bool UKnsAbilitySystemComponent::TryConsumeOnom(float Amount)
{
	if (!HasEnoughOnom(Amount))
	{
		return false;
	}

	ApplyResourceChange(TEXT("Resource.Onom"), -Amount);
	return true;
}

void UKnsAbilitySystemComponent::RegenerateOnom(float Amount)
{
	if (Amount <= 0.f)
	{
		return;
	}

	const float Missing = FMath::Max(0.f, GetMaxOnom() - GetOnom());
	if (Missing > 0.f)
	{
		ApplyResourceChange(TEXT("Resource.Onom"), FMath::Min(Amount, Missing));
	}
}

float UKnsAbilitySystemComponent::GetFocus() const
{
	return GetAttributeValue(UKnsPlayerAttributeSet::GetFocusAttribute());
}

float UKnsAbilitySystemComponent::GetMaxFocus() const
{
	return GetAttributeValue(UKnsPlayerAttributeSet::GetMaxFocusAttribute());
}

bool UKnsAbilitySystemComponent::HasEnoughFocus(float Amount) const
{
	return Amount <= 0.f || GetFocus() >= Amount;
}

bool UKnsAbilitySystemComponent::TryConsumeFocus(float Amount)
{
	if (!HasEnoughFocus(Amount))
	{
		return false;
	}

	ApplyResourceChange(TEXT("Resource.Focus"), -Amount);
	return true;
}

void UKnsAbilitySystemComponent::RegenerateFocus(float Amount)
{
	if (Amount <= 0.f)
	{
		return;
	}

	const float Missing = FMath::Max(0.f, GetMaxFocus() - GetFocus());
	if (Missing > 0.f)
	{
		ApplyResourceChange(TEXT("Resource.Focus"), FMath::Min(Amount, Missing));
	}
}

void UKnsAbilitySystemComponent::ApplyDamageToActor(AActor* TargetActor, float MoveMultiplier, float CritMultiplier)
{
	if (!TargetActor)
	{
		return;
	}

	UAbilitySystemComponent* TargetASC = nullptr;
	if (IAbilitySystemInterface* TargetInterface = Cast<IAbilitySystemInterface>(TargetActor))
	{
		TargetASC = TargetInterface->GetAbilitySystemComponent();
	}

	if (!TargetASC)
	{
		return;
	}

	FGameplayEffectContextHandle Context = MakeEffectContext();
	Context.AddInstigator(GetOwnerActor(), GetOwnerActor());

	FGameplayEffectSpecHandle Handle = MakeOutgoingSpec(UKnsDamageGameplayEffect::StaticClass(), 1.f, Context);
	if (!Handle.IsValid())
	{
		return;
	}

	Handle.Data->SetSetByCallerMagnitude(TEXT("Damage.Multiplier"), MoveMultiplier);
	Handle.Data->SetSetByCallerMagnitude(TEXT("Damage.CritMultiplier"), CritMultiplier);
	TargetASC->ApplyGameplayEffectSpecToSelf(*Handle.Data.Get());

	if (const UKnsCommonAttributeSet* TargetCommon = TargetASC->GetSet<UKnsCommonAttributeSet>())
	{
		if (TargetCommon->GetHealth() <= 0.f)
		{
			const FGameplayTag DeadTag = FGameplayTag::RequestGameplayTag(TEXT("Status.Dead"), false);
			if (DeadTag.IsValid() && !TargetASC->HasMatchingGameplayTag(DeadTag))
			{
				TargetASC->AddLooseGameplayTag(DeadTag);
				if (UKnsAbilitySystemComponent* TargetKnsASC = Cast<UKnsAbilitySystemComponent>(TargetASC))
				{
					TargetKnsASC->OnActorDied.Broadcast(TargetActor);
				}
			}
		}
	}
}

void UKnsAbilitySystemComponent::ApplyFlatDamageToActor(AActor* TargetActor, float Damage)
{
	if (!TargetActor || Damage <= 0.f)
	{
		return;
	}

	UAbilitySystemComponent* TargetASC = nullptr;
	if (IAbilitySystemInterface* TargetInterface = Cast<IAbilitySystemInterface>(TargetActor))
	{
		TargetASC = TargetInterface->GetAbilitySystemComponent();
	}

	if (!TargetASC)
	{
		return;
	}

	FGameplayEffectContextHandle Context = MakeEffectContext();
	Context.AddInstigator(GetOwnerActor(), GetOwnerActor());

	FGameplayEffectSpecHandle Handle = MakeOutgoingSpec(UKnsDamageGameplayEffect::StaticClass(), 1.f, Context);
	if (!Handle.IsValid())
	{
		return;
	}

	Handle.Data->SetSetByCallerMagnitude(TEXT("Damage.Flat"), Damage);
	Handle.Data->SetSetByCallerMagnitude(TEXT("Damage.Multiplier"), 1.f);
	Handle.Data->SetSetByCallerMagnitude(TEXT("Damage.CritMultiplier"), 1.f);
	TargetASC->ApplyGameplayEffectSpecToSelf(*Handle.Data.Get());

	if (const UKnsCommonAttributeSet* TargetCommon = TargetASC->GetSet<UKnsCommonAttributeSet>())
	{
		if (TargetCommon->GetHealth() <= 0.f)
		{
			const FGameplayTag DeadTag = FGameplayTag::RequestGameplayTag(TEXT("Status.Dead"), false);
			if (DeadTag.IsValid() && !TargetASC->HasMatchingGameplayTag(DeadTag))
			{
				TargetASC->AddLooseGameplayTag(DeadTag);
				if (UKnsAbilitySystemComponent* TargetKnsASC = Cast<UKnsAbilitySystemComponent>(TargetASC))
				{
					TargetKnsASC->OnActorDied.Broadcast(TargetActor);
				}
			}
		}
	}
}

bool UKnsAbilitySystemComponent::ApplyHitToActor(AActor* TargetActor, const FRHHitData& HitData)
{
	if (!TargetActor)
	{
		return false;
	}

	UAbilitySystemComponent* TargetASC = nullptr;
	if (IAbilitySystemInterface* TargetInterface = Cast<IAbilitySystemInterface>(TargetActor))
	{
		TargetASC = TargetInterface->GetAbilitySystemComponent();
	}

	if (!TargetASC)
	{
		return false;
	}

	// 已经死亡的目标不再吃后续命中，避免打断死亡蒙太奇。
	const FGameplayTag InitialDeadTag = FGameplayTag::RequestGameplayTag(TEXT("Status.Dead"), false);
	if (InitialDeadTag.IsValid() && TargetASC->HasMatchingGameplayTag(InitialDeadTag))
	{
		return false;
	}

	// 无敌帧（闪避/倒地等）：整次命中跳过（不受伤/不吃音形惩罚/不触发受击）。
	const FGameplayTag InvincibleTag = FGameplayTag::RequestGameplayTag(TEXT("Status.Invincible"), false);
	if (InvincibleTag.IsValid() && TargetASC->HasMatchingGameplayTag(InvincibleTag))
	{
		// 玩家闪避中被打：全局时间膨胀短暂变缓再恢复（敌人不触发）。
		if (URHCombatComponent* RHCombat = TargetActor->FindComponentByClass<URHCombatComponent>())
		{
			if (RHCombat->GetActionState() == ERHActionState::Dodge)
			{
				RHCombat->TriggerDodgeTimeDilation();
				RHCombat->HandleDodgeSuccess();
			}
		}
		return false;
	}

	const FGameplayTag GuardingTag = FGameplayTag::RequestGameplayTag(TEXT("Status.Guarding"), false);
	const bool bGuarding = GuardingTag.IsValid() && TargetASC->HasMatchingGameplayTag(GuardingTag);

	URHEnemyAIComponent* EnemyAI = TargetActor->FindComponentByClass<URHEnemyAIComponent>();

	// 防御受击同样按招式 CounterBarDamage 扣反击条（与普通命中一致）。
	// 反击条归零时吞掉当前命中，让 StateTree 能进 Deflect 而不是同时被打进受击硬直；
	// 之后的命中只有实际 deflect window 打开时才被弹开。
	if (EnemyAI && !EnemyAI->IsBroken() && !EnemyAI->IsDeflectWindowActive())
	{
		const bool bDeflectTriggered = EnemyAI->DrainCounterBarOnPlayerHit(HitData.CounterBarDamage, HitData.Source ? HitData.Source : GetOwnerActor());
		if (bDeflectTriggered)
		{
			// 反击条清空即弹开：无论是普通命中还是防御受击，都在这帧直接走 Deflect，
			// 不再继续跑守卫受击/破防结算，避免同帧叠加两条蒙太奇崩溃。
			return false;
		}
	}

	// Enemy deflect window: this player hit is deflected, player gets a small stagger, enemy takes its turn.
	if (EnemyAI && EnemyAI->IsDeflectWindowActive())
	{
		EnemyAI->NotifyDeflectSuccess(HitData.Source ? HitData.Source : GetOwnerActor());
		return false;
	}

	UKnsCombatComponent* TargetCombat = TargetActor->FindComponentByClass<UKnsCombatComponent>();
	UKnsCombatComponent* AttackerCombat = GetOwnerActor() ? GetOwnerActor()->FindComponentByClass<UKnsCombatComponent>() : nullptr;
	URHCombatComponent* TargetRHCombat = Cast<URHCombatComponent>(TargetCombat);

	// Just Load / Just Reload / Reverse Just Reload 窗口：被击中 → 执行对应动作（load/逆装填/逆转逆装填）+ 命中无效化；
	// 前置条件不满足（无 onom / 无共鸣槽）返回 false，命中照常结算（窗口空转，不无敌）。
	if (TargetRHCombat && TargetRHCombat->TryHandleJustWindowHit(HitData))
	{
		return false;
	}

	const bool bTargetBlocking = TargetRHCombat && TargetRHCombat->IsBlocking();
	const bool bPerfectGuardWindow = TargetCombat && TargetCombat->IsPerfectGuardWindowActive();
	bool bDefensiveOutcome = false;
	bool bPlayedParryFeedback = false;

	if (TargetCombat && (bGuarding || bPerfectGuardWindow))
	{
		const ERHOnomGuardOutcome GuardOutcome = TargetCombat->ResolveGuardHit(bPerfectGuardWindow);
		if (GuardOutcome == ERHOnomGuardOutcome::BigBreak)
		{
			// 大破防：算防御成功，只吃固定伤害，不吃来袭伤害。
			PlayHitFeedback(AttackerCombat, true, HitData, TargetActor);
			TargetCombat->ApplyBigBreak(HitData.Source ? HitData.Source : GetOwnerActor());
			return true;
		}
		if (!bPerfectGuardWindow)
		{
			TargetCombat->PlayDefensiveCameraShake(ERHDefensiveShakeType::GuardHit);
		}
		bDefensiveOutcome = true;
	}

	// 完美防御/弹反反馈：固定音效 + 按本次获得的 Onom 极性选特效（用防御方自己的反馈 DA）。
	if (bPerfectGuardWindow)
	{
		ERHOnomPolarity ParryPolarity = ERHOnomPolarity::None;
		if (URHWeaponDefinition* WeaponDef = TargetCombat ? TargetCombat->WeaponDefinition : nullptr)
		{
			switch (WeaponDef->PerfectGuardHitRule.Type)
			{
			case ERHOnomValue::Positive: ParryPolarity = ERHOnomPolarity::Major; break;
			case ERHOnomValue::Negative: ParryPolarity = ERHOnomPolarity::Minor; break;
			case ERHOnomValue::Broken: ParryPolarity = ERHOnomPolarity::Neutral; break;
			default: break;
			}
		}
		PlayParryFeedback(TargetCombat, ParryPolarity, HitData);
		bPlayedParryFeedback = true;
	}

	// 弹反成功（完美窗口接中命中，且目标正在播弹反动作）：玩家直接进自由态，下次弹反换手。
	if (bPerfectGuardWindow)
	{
		if (TargetRHCombat && TargetRHCombat->GetActionState() == ERHActionState::Parry)
		{
			TargetRHCombat->NotifyParrySuccess();
		}
	}

	// 格挡/弹反成功期间不扣血（防御结算照常：灰色积累/弹反奖励），大破防固定伤害由 ApplyBigBreak 另算。
	if (HitData.Damage > 0.f && !bTargetBlocking && !bPerfectGuardWindow && !bDefensiveOutcome)
	{
		float FinalDamage = HitData.Damage;
		if (EnemyAI && EnemyAI->IsBroken())
		{
			// 破防期间全程增伤（进入破防任务即生效）。
			FinalDamage *= EnemyAI->GetBreakDamageMultiplier();
		}
		if (TargetCombat && !bDefensiveOutcome)
		{
			// 执行中招式的减伤系数（独立于打断判定）。
			FinalDamage *= (1.f - FMath::Clamp(TargetCombat->GetCurrentResistance(), 0.f, 1.f));
		}

		FGameplayEffectContextHandle Context = MakeEffectContext();
		Context.AddInstigator(GetOwnerActor(), HitData.Source ? HitData.Source : GetOwnerActor());

		FGameplayEffectSpecHandle Handle = MakeOutgoingSpec(UKnsDamageGameplayEffect::StaticClass(), 1.f, Context);
		if (Handle.IsValid())
		{
			Handle.Data->SetSetByCallerMagnitude(TEXT("Damage.Flat"), FinalDamage);
			Handle.Data->SetSetByCallerMagnitude(TEXT("Damage.Multiplier"), 1.f);
			Handle.Data->SetSetByCallerMagnitude(TEXT("Damage.CritMultiplier"), 1.f);
			TargetASC->ApplyGameplayEffectSpecToSelf(*Handle.Data.Get());

		if (EnemyAI && EnemyAI->IsBroken())
		{
			// 破防（处决等待）期间只打血、不再扣共振：
			// 原逻辑 ApplyBreakDamage 会把共振扣到 0 → HandleBreakDepleted 解除破防 + 播起身，
			// 同一帧受击打断判定 !IsBroken() 成立 → 发 HitTaken → 把处决等待态顶掉（敌人无法被处决）。
			// 破防解除只由自然衰减（DecayPerSecondDuringBreak）或处决完成（FinishExecution）驱动。
			// EnemyAI->ApplyBreakDamage(FinalDamage);
		}
		}
	}

	if (HitData.ResonanceDamage > 0.f)
	{
		const float GuardMultiplier = bGuarding ? FMath::Max(HitData.GuardResonanceMultiplier, 0.f) : 1.f;
		if (EnemyAI)
		{
			// Enemy resonance fills from 0 to Max; full means guard break.
			EnemyAI->AddResonance(HitData.ResonanceDamage * GuardMultiplier
				* EnemyAI->GetCurrentConfig().Resonance.ResonanceGainScale);
		}
		else
		{
			// 非 AI 敌人：共振从 0 增长，打满破防。
			ApplyResourceChangeToActor(TargetActor, TEXT("Resource.Resonance"), HitData.ResonanceDamage * GuardMultiplier);
		}
	}

	if (!bDefensiveOutcome && HitData.bApplyOnomPenalty)
	{
		if (URHOnomComponent* TargetOnom = TargetActor->FindComponentByClass<URHOnomComponent>())
		{
			TargetOnom->ApplyDamageTakenRule(GetOwnerActor());
		}
	}

	if (const UKnsCommonAttributeSet* TargetCommon = TargetASC->GetSet<UKnsCommonAttributeSet>())
	{
		const FGameplayTag DeadTag = FGameplayTag::RequestGameplayTag(TEXT("Status.Dead"), false);
		if (TargetCommon->GetHealth() <= 0.f && DeadTag.IsValid() && !TargetASC->HasMatchingGameplayTag(DeadTag))
		{
			TargetASC->AddLooseGameplayTag(DeadTag);
			if (UKnsAbilitySystemComponent* TargetKnsASC = Cast<UKnsAbilitySystemComponent>(TargetASC))
			{
				TargetKnsASC->OnActorDied.Broadcast(TargetActor);
			}
		}

		const FGameplayTag StaggeredTag = FGameplayTag::RequestGameplayTag(TEXT("Status.Staggered"), false);
		if (!EnemyAI && TargetCommon->GetResonance() >= TargetCommon->GetMaxResonance() && StaggeredTag.IsValid() && !TargetASC->HasMatchingGameplayTag(StaggeredTag))
		{
			TargetASC->AddLooseGameplayTag(StaggeredTag);
			ApplyResourceChangeToActor(TargetActor, TEXT("Resource.Resonance"), -TargetCommon->GetResonance());
			if (UKnsAbilitySystemComponent* TargetKnsASC = Cast<UKnsAbilitySystemComponent>(TargetASC))
			{
				TargetKnsASC->OnResonanceBroken.Broadcast(TargetActor);
			}
		}
	}

	if (!bPlayedParryFeedback)
	{
		PlayHitFeedback(AttackerCombat, bDefensiveOutcome, HitData, TargetActor);
	}

	// 受击判定：常态/防御态/破防中都播放（防御态走防御受击/破防受击），完美弹反窗口除外。
	if (TargetCombat && HitData.PoiseLevel > 0 && !bPerfectGuardWindow)
	{
		if (HitData.bUseManualHitDirection)
		{
			// 手动标记方向（hitbox tag F/L/R/B）：直接播对应方向受击动画。
			TargetCombat->HandleIncomingHit(HitData.HitDirection, HitData.PoiseLevel, HitData.Source ? HitData.Source : GetOwnerActor());
		}
		else
		{
			TargetCombat->HandleIncomingHitAt(HitData.HitLocation, HitData.Source ? HitData.Source : GetOwnerActor(), HitData.PoiseLevel);
		}
	}
	return true;
}

void UKnsAbilitySystemComponent::ApplyPoiseDamageToActor(AActor* TargetActor, float Amount)
{
	if (!TargetActor || Amount <= 0.f)
	{
		return;
	}

	UAbilitySystemComponent* TargetASC = nullptr;
	if (IAbilitySystemInterface* TargetInterface = Cast<IAbilitySystemInterface>(TargetActor))
	{
		TargetASC = TargetInterface->GetAbilitySystemComponent();
	}

	if (!TargetASC)
	{
		return;
	}

	FGameplayEffectContextHandle Context = MakeEffectContext();
	FGameplayEffectSpecHandle Handle = MakeOutgoingSpec(UKnsResourceGameplayEffect::StaticClass(), 1.f, Context);
	if (!Handle.IsValid())
	{
		return;
	}

	Handle.Data->SetSetByCallerMagnitude(TEXT("Resource.Stamina"), 0.f);
	Handle.Data->SetSetByCallerMagnitude(TEXT("Resource.Onom"), 0.f);
	Handle.Data->SetSetByCallerMagnitude(TEXT("Resource.Focus"), 0.f);
	Handle.Data->SetSetByCallerMagnitude(TEXT("Resource.Resonance"), 0.f);
	Handle.Data->SetSetByCallerMagnitude(TEXT("Resource.Poise"), -Amount);
	TargetASC->ApplyGameplayEffectSpecToSelf(*Handle.Data.Get());
}

float UKnsAbilitySystemComponent::GetResonance() const
{
	return GetAttributeValue(UKnsCommonAttributeSet::GetResonanceAttribute());
}

float UKnsAbilitySystemComponent::GetMaxResonance() const
{
	return GetAttributeValue(UKnsCommonAttributeSet::GetMaxResonanceAttribute());
}

void UKnsAbilitySystemComponent::RegenerateResonance(float Amount)
{
	if (Amount <= 0.f)
	{
		return;
	}

	const float Missing = FMath::Max(0.f, GetMaxResonance() - GetResonance());
	if (Missing > 0.f)
	{
		ApplyResourceChange(TEXT("Resource.Resonance"), FMath::Min(Amount, Missing));
	}
}

void UKnsAbilitySystemComponent::ResetResonance()
{
	ApplyResonanceDelta(-GetResonance());
}

void UKnsAbilitySystemComponent::ApplyResonanceDelta(float Amount)
{
	if (Amount == 0.f)
	{
		return;
	}

	const float Current = GetResonance();
	const float NewValue = FMath::Clamp(Current + Amount, 0.f, GetMaxResonance());
	if (!FMath::IsNearlyEqual(NewValue, Current))
	{
		ApplyResourceChange(TEXT("Resource.Resonance"), NewValue - Current);
	}
}

void UKnsAbilitySystemComponent::ApplyResourceChange(FName ResourceName, float Amount)
{
	ApplyResourceChangeToActor(GetOwnerActor(), ResourceName, Amount);
}

void UKnsAbilitySystemComponent::ApplyResourceChangeToActor(AActor* TargetActor, FName ResourceName, float Amount)
{
	if (!TargetActor)
	{
		return;
	}

	UAbilitySystemComponent* TargetASC = nullptr;
	if (IAbilitySystemInterface* TargetInterface = Cast<IAbilitySystemInterface>(TargetActor))
	{
		TargetASC = TargetInterface->GetAbilitySystemComponent();
	}

	if (!TargetASC)
	{
		return;
	}

	FGameplayEffectContextHandle Context = MakeEffectContext();
	FGameplayEffectSpecHandle Handle = MakeOutgoingSpec(UKnsResourceGameplayEffect::StaticClass(), 1.f, Context);
	if (!Handle.IsValid())
	{
		return;
	}

	Handle.Data->SetSetByCallerMagnitude(TEXT("Resource.Stamina"), ResourceName == TEXT("Resource.Stamina") ? Amount : 0.f);
	Handle.Data->SetSetByCallerMagnitude(TEXT("Resource.Onom"), ResourceName == TEXT("Resource.Onom") ? Amount : 0.f);
	Handle.Data->SetSetByCallerMagnitude(TEXT("Resource.Focus"), ResourceName == TEXT("Resource.Focus") ? Amount : 0.f);
	Handle.Data->SetSetByCallerMagnitude(TEXT("Resource.Poise"), ResourceName == TEXT("Resource.Poise") ? Amount : 0.f);
	Handle.Data->SetSetByCallerMagnitude(TEXT("Resource.Resonance"), ResourceName == TEXT("Resource.Resonance") ? Amount : 0.f);
	TargetASC->ApplyGameplayEffectSpecToSelf(*Handle.Data.Get());
}
