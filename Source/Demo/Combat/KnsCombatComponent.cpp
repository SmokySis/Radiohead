#include "KnsCombatComponent.h"

#include "AbilitySystemInterface.h"
#include "Animation/AnimInstance.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Demo/Combat/Weapon/AWeaponBase.h"
#include "Demo/Combat/KnsHitReactionSettingsDataAsset.h"
#include "Demo/Combat/KnsHitStopSettingsDataAsset.h"
#include "Demo/Combat/KnsMoveDefinition.h"
#include "Demo/Combat/KnsCombatContextComponent.h"
#include "Demo/Combat/RHHitData.h"
#include "Demo/Combat/RHCombatComponent.h"
#include "Demo/Character/BaseCharacter.h"
#include "Demo/Character/RHEnemyBase.h"
#include "Demo/Debug/KnsCombatDebugSubsystem.h"
#include "Demo/GAS/KnsAbilitySystemComponent.h"
#include "Demo/GAS/KnsCommonAttributeSet.h"
#include "Demo/GAS/KnsPlayerAttributeSet.h"
#include "Demo/Onom/RHOnomComponent.h"
#include "Demo/Onom/RHOnomSettings.h"
#include "Demo/Onom/RHOnomActionDefinition.h"
#include "Demo/Onom/RHCoreDefinition.h"
#include "Demo/Onom/RHDodgeCoreDefinition.h"
#include "Demo/Combat/RHMoveDefinition.h"
#include "Demo/Onom/RHWeaponDefinition.h"
#include "Animation/AnimMontage.h"
#include "GameFramework/GameplayCameraComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "TimerManager.h"

namespace
{
	EKnsHitDirection ComputeHitDirectionFromPoint(const FVector& Point, const AActor* Target)
	{
		if (!Target)
		{
			return EKnsHitDirection::Front;
		}

		const FVector ToPoint = Point - Target->GetActorLocation();
		const FVector Dir = ToPoint.GetSafeNormal2D();
		if (Dir.IsNearlyZero())
		{
			return EKnsHitDirection::Front;
		}

		const FVector Forward = Target->GetActorForwardVector();
		const FVector Right = Target->GetActorRightVector();
		const float FwdDot = FVector::DotProduct(Forward, Dir);
		const float RightDot = FVector::DotProduct(Right, Dir);

		if (FMath::Abs(FwdDot) >= FMath::Abs(RightDot))
		{
			return FwdDot >= 0.f ? EKnsHitDirection::Front : EKnsHitDirection::Back;
		}
		return RightDot >= 0.f ? EKnsHitDirection::Right : EKnsHitDirection::Left;
	}
}

bool UKnsCombatComponent::ResolveHitDirectionTag(const FGameplayTag& Tag, EKnsHitDirection& OutDirection)
{
	if (!Tag.IsValid())
	{
		return false;
	}

	const FName Name = Tag.GetTagName();
	if (Name == TEXT("HitReaction.Direction.F") || Name == TEXT("HitReaction.Direction.Front"))
	{
		OutDirection = EKnsHitDirection::Front;
		return true;
	}
	if (Name == TEXT("HitReaction.Direction.B") || Name == TEXT("HitReaction.Direction.Back"))
	{
		OutDirection = EKnsHitDirection::Back;
		return true;
	}
	if (Name == TEXT("HitReaction.Direction.L") || Name == TEXT("HitReaction.Direction.Left"))
	{
		OutDirection = EKnsHitDirection::Left;
		return true;
	}
	if (Name == TEXT("HitReaction.Direction.R") || Name == TEXT("HitReaction.Direction.Right"))
	{
		OutDirection = EKnsHitDirection::Right;
		return true;
	}
	return false;
}

UKnsCombatComponent::UKnsCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	CastFlashComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("CastFlash"));
	CastFlashComponent->SetAutoActivate(false);
}

void UKnsCombatComponent::BeginPlay()
{
	Super::BeginPlay();
	ApplyAttributeInitializer();
	SpawnAndAttachWeapon();

	if (ABaseCharacter* Character = Cast<ABaseCharacter>(GetOwner()))
	{
		if (UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance())
		{
			AnimInstance->OnMontageEnded.AddDynamic(this, &UKnsCombatComponent::HandleReactionMontageEnded);
		}
		if (UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement())
		{
			DefaultMaxWalkSpeed = MoveComp->MaxWalkSpeed;
		}
		if (CastFlashComponent)
		{
			CastFlashComponent->AttachToComponent(Character->GetMesh(), FAttachmentTransformRules::KeepRelativeTransform, CastFlashSocket);
		}

		Character->GetCapsuleComponent()->OnComponentHit.AddDynamic(this, &UKnsCombatComponent::HandleBodyHit);
	}

	if (AActor* Owner = GetOwner())
	{
		CombatContext = Owner->FindComponentByClass<UKnsCombatContextComponent>();
		CachedOnomComponent = Owner->FindComponentByClass<URHOnomComponent>();
	}
}

void UKnsCombatComponent::ApplyAttributeInitializer()
{
	if (!AttributeInitializer.bApplyOnBeginPlay)
	{
		return;
	}

	UKnsAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}

	if (UKnsCommonAttributeSet* CommonAS = const_cast<UKnsCommonAttributeSet*>(ASC->GetSet<UKnsCommonAttributeSet>()))
	{
		CommonAS->InitMaxHealth(AttributeInitializer.MaxHealth);
		CommonAS->InitHealth(AttributeInitializer.Health);
		CommonAS->InitMaxStamina(AttributeInitializer.MaxStamina);
		CommonAS->InitStamina(AttributeInitializer.Stamina);
		CommonAS->InitAttackPower(AttributeInitializer.AttackPower);
		CommonAS->InitCritRate(AttributeInitializer.CritRate);
		CommonAS->InitResistance(AttributeInitializer.Resistance);
		CommonAS->InitDefense(AttributeInitializer.Defense);
		CommonAS->InitDamageReduction(AttributeInitializer.DamageReduction);
		CommonAS->InitMaxPoise(AttributeInitializer.MaxPoise);
		CommonAS->InitPoise(AttributeInitializer.Poise);
		CommonAS->InitMaxResonance(AttributeInitializer.MaxResonance);
		CommonAS->InitResonance(AttributeInitializer.Resonance);
	}

	if (UKnsPlayerAttributeSet* PlayerAS = const_cast<UKnsPlayerAttributeSet*>(ASC->GetSet<UKnsPlayerAttributeSet>()))
	{
		PlayerAS->InitMaxOnom(AttributeInitializer.MaxOnom);
		PlayerAS->InitOnom(AttributeInitializer.Onom);
		PlayerAS->InitMaxFocus(AttributeInitializer.MaxFocus);
		PlayerAS->InitFocus(AttributeInitializer.Focus);
	}
}

AWeaponBase* UKnsCombatComponent::SpawnAndAttachWeapon()
{
	if (SpawnedWeapon)
	{
		return SpawnedWeapon;
	}

	ABaseCharacter* Character = Cast<ABaseCharacter>(GetOwner());
	// 武器类/主手 Socket 从当前武器 DA 取（DA 未配武器类则不生成）。
	if (!Character || !WeaponDefinition || !WeaponDefinition->WeaponClass || !GetWorld())
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = Character;
	SpawnParameters.Instigator = Character->GetInstigator();
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	SpawnedWeapon = GetWorld()->SpawnActor<AWeaponBase>(
		WeaponDefinition->WeaponClass,
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		SpawnParameters);

	if (SpawnedWeapon)
	{
		SpawnedWeapon->AttachToComponent(
			Character->GetMesh(),
			FAttachmentTransformRules::SnapToTargetNotIncludingScale,
			WeaponDefinition->WeaponSocketName);

		// 双刀：副刀身挂到副 socket（资产未配时复用主刀外观）。
		SpawnedWeapon->AttachSecondaryMesh(Character->GetMesh());

		// 主副命中框都参与判定（共用同一套 ActiveHitboxTag/HitActorsThisHitbox 状态）；副框仅双刀（配了副手 socket）启用。
		if (UBoxComponent* HitboxBox = SpawnedWeapon->GetHitboxBox())
		{
			HitboxBox->OnComponentBeginOverlap.AddDynamic(this, &UKnsCombatComponent::HandleHitboxOverlap);
		}
		if (SpawnedWeapon->HasSecondaryBlade())
		{
			if (UBoxComponent* SecondaryHitboxBox = SpawnedWeapon->GetSecondaryHitboxBox())
			{
				SecondaryHitboxBox->OnComponentBeginOverlap.AddDynamic(this, &UKnsCombatComponent::HandleHitboxOverlap);
			}
		}
	}

	return SpawnedWeapon;
}

void UKnsCombatComponent::DestroySpawnedWeapon()
{
	if (SpawnedWeapon)
	{
		SpawnedWeapon->Destroy();
		SpawnedWeapon = nullptr;
	}
}

bool UKnsCombatComponent::SwitchWeapon(URHWeaponDefinition* NewWeaponDefinition)
{
	// 武器类/主手 Socket 从 DA 取；DA 未配武器类则不换。
	if (!NewWeaponDefinition || !NewWeaponDefinition->WeaponClass)
	{
		return false;
	}

	DestroySpawnedWeapon();
	WeaponDefinition = NewWeaponDefinition;

	// 切武器重置共鸣衰减速率回 1.0（新武器默认；已有共鸣不再沿用旧武器的衰减倍率）。
	if (URHOnomComponent* Onom = GetOnomComponent())
	{
		Onom->SetResonanceDecayRate(1.f);
	}

	return SpawnAndAttachWeapon() != nullptr;
}

void UKnsCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bIsRunning)
	{
		UKnsAbilitySystemComponent* ASC = GetAbilitySystemComponent();
		if (!ASC || !ASC->TryConsumeStamina(RunStaminaCostPerSecond * DeltaTime))
		{
			bIsRunning = false;
			OnRunExhausted.Broadcast();
		}
	}

	if (bRotationInterpActive)
	{
		if (AActor* Owner = GetOwner())
		{
			const FRotator CurrentRotation = Owner->GetActorRotation();
			const FRotator NewRotation = FMath::RInterpTo(CurrentRotation, RotationInterpTarget, DeltaTime, RotationInterpSpeed);
			Owner->SetActorRotation(NewRotation);

			if (CurrentRotation.Equals(RotationInterpTarget, 0.5f))
			{
				bRotationInterpActive = false;
			}
		}
		else
		{
			bRotationInterpActive = false;
		}
	}
}

bool UKnsCombatComponent::TryConsumeRollStamina()
{
	UKnsAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	return ASC && ASC->TryConsumeStamina(RollStaminaCost);
}

bool UKnsCombatComponent::TryConsumeDefendStamina()
{
	UKnsAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	return ASC && ASC->TryConsumeStamina(DefendStaminaCost);
}

void UKnsCombatComponent::SetRunning(bool bInRunning)
{
	bIsRunning = bInRunning;
}

bool UKnsCombatComponent::IsRunning() const
{
	return bIsRunning;
}

void UKnsCombatComponent::StartRotateToMoveInput(const FRotator& InTargetRotation, float InInterpSpeed)
{
	RotationInterpTarget = InTargetRotation;
	RotationInterpSpeed = InInterpSpeed;
	bRotationInterpActive = InInterpSpeed > 0.f;

	if (!bRotationInterpActive)
	{
		if (AActor* Owner = GetOwner())
		{
			Owner->SetActorRotation(InTargetRotation);
		}
	}
}

void UKnsCombatComponent::SetTouchConditionEnabled(bool bEnabled)
{
	bTouchConditionEnabled = bEnabled;
}

bool UKnsCombatComponent::CanRollCancel() const
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC)
	{
		return true;
	}

	const FGameplayTag AttackingTag = FGameplayTag::RequestGameplayTag(TEXT("State.Player.Attacking"), false);
	const FGameplayTag RollCancelTag = FGameplayTag::RequestGameplayTag(TEXT("Combo.Cancel.Roll"), false);

	return !ASC->HasMatchingGameplayTag(AttackingTag) || ASC->HasMatchingGameplayTag(RollCancelTag);
}

void UKnsCombatComponent::HandleIncomingHit(EKnsHitDirection Direction, int32 AttackPoiseLevel, AActor* AttackSource)
{
	UKnsAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (ASC)
	{
		const FGameplayTag InvincibleTag = FGameplayTag::RequestGameplayTag(TEXT("Status.Invincible"), false);
		if (ASC->HasMatchingGameplayTag(InvincibleTag))
		{
			return;
		}
	}

	// 防御态韧性 = 0；非防御取当前招式韧性。
	const bool bGuardingNow = bGuarding
		|| (ASC && ASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("Status.Guarding"), false)));
	const int32 CurrentPoiseLevel = bGuardingNow ? 0 : (CombatContext ? CombatContext->GetEffectivePoiseLevel() : 0);

	// 防御态必播防御受击/破防受击；非防御态攻击韧性不大于当前招式韧性时不打断。
	if (!bGuardingNow && AttackPoiseLevel <= CurrentPoiseLevel)
	{
		return;
	}

	if (CombatContext && CombatContext->IsComboActive())
	{
		CombatContext->RequestCancelCombo();
	}

	const int32 Diff = AttackPoiseLevel - CurrentPoiseLevel;
	EKnsHitReactionStrength Strength = EKnsHitReactionStrength::Light;
	if (HitReactionThresholds)
	{
		// 韧性区间由 DA 配置（差值阈值对应轻/中/重/倒地）。
		Strength = HitReactionThresholds->GetStrengthForPoiseDiff(Diff);
	}
	else if (Diff >= 4)
	{
		Strength = EKnsHitReactionStrength::Knockdown;
	}
	else if (Diff == 3)
	{
		Strength = EKnsHitReactionStrength::Heavy;
	}
	else if (Diff == 2)
	{
		Strength = EKnsHitReactionStrength::Medium;
	}

	PlayHitReaction(Direction, Strength, AttackPoiseLevel, CurrentPoiseLevel, AttackSource);
}

void UKnsCombatComponent::HandleIncomingHitFrom(AActor* Source, int32 AttackPoiseLevel)
{
	const FVector Point = Source ? Source->GetActorLocation() : FVector::ZeroVector;
	const EKnsHitDirection Direction = ComputeHitDirectionFromPoint(Point, GetOwner());
	HandleIncomingHit(Direction, AttackPoiseLevel, Source);
}

void UKnsCombatComponent::HandleIncomingHitAt(FVector HitLocation, AActor* FallbackSource, int32 AttackPoiseLevel)
{
	FVector Point = HitLocation;
	if (Point.IsNearlyZero() && FallbackSource)
	{
		Point = FallbackSource->GetActorLocation();
	}
	const EKnsHitDirection Direction = ComputeHitDirectionFromPoint(Point, GetOwner());
	HandleIncomingHit(Direction, AttackPoiseLevel, FallbackSource);
}

float UKnsCombatComponent::GetCurrentResistance() const
{
	return CombatContext ? CombatContext->GetResistance() : 0.f;
}

void UKnsCombatComponent::EndKnockdown()
{
	if (UKnsAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		const FGameplayTag InvincibleTag = FGameplayTag::RequestGameplayTag(TEXT("Status.Invincible"), false);
		const FGameplayTag KnockedDownTag = FGameplayTag::RequestGameplayTag(TEXT("Status.KnockedDown"), false);
		if (InvincibleTag.IsValid())
		{
			ASC->RemoveLooseGameplayTag(InvincibleTag);
		}
		if (KnockedDownTag.IsValid())
		{
			ASC->RemoveLooseGameplayTag(KnockedDownTag);
		}
	}
}

void UKnsCombatComponent::DebugTriggerHitReaction(EKnsHitDirection Direction, int32 AttackPoiseLevel)
{
	HandleIncomingHit(Direction, AttackPoiseLevel);
}

void UKnsCombatComponent::PlayHitReaction(EKnsHitDirection Direction, EKnsHitReactionStrength Strength, int32 AttackPoiseLevel, int32 CurrentPoiseLevel, AActor* AttackSource)
{
	OnHitReceived.Broadcast(Direction, Strength, AttackPoiseLevel, CurrentPoiseLevel);

	// 受击已广播给监听者（如敌人破防中转处刑）；若覆写要求跳过，则不再播受击蒙太奇。
	if (ShouldSkipHitReaction())
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		if (UKnsCombatDebugSubsystem* DebugSubsystem = World->GetSubsystem<UKnsCombatDebugSubsystem>())
		{
			DebugSubsystem->LogEvent(TEXT("HitReaction"), FColor::Magenta, FString::Printf(TEXT("Dir=%d Strength=%d"), (int32)Direction, (int32)Strength));
		}
	}

	UKnsHitReactionSettingsDataAsset* ReactionSettings = GetHitReactionData();
	if (!ReactionSettings)
	{
		return;
	}

	const bool bGuardingNow = bGuarding
		|| (GetAbilitySystemComponent()
			&& GetAbilitySystemComponent()->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("Status.Guarding"), false)));
	const bool bKnockdown = (Strength == EKnsHitReactionStrength::Knockdown);

	// 防御态：轻/中 → 防御受击；重 → 破防受击；倒地级 → 直接击飞。
	UAnimMontage* Montage = nullptr;
	if (bGuardingNow && !bKnockdown)
	{
		Montage = (Strength == EKnsHitReactionStrength::Light || Strength == EKnsHitReactionStrength::Medium)
			? ReactionSettings->GetDefensiveHitMontage()
			: ReactionSettings->GetDefensiveBreakMontage();
	}
	else
	{
		Montage = ReactionSettings->GetReactionMontage(Strength, Direction);
	}
	if (!Montage)
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

	StopCurrentReactionMontage();
	CurrentReactionMontage = Montage;

	// RH 角色：受击打断当前动作（MH 侧由 ComboContext 处理取消）。
	if (URHCombatComponent* RHCombat = Cast<URHCombatComponent>(this))
	{
		RHCombat->CancelAction();
		// 受击动画期间挂 Busy 状态：不可被其它动作随意打断（打断由受击蒙太奇上的 RH Cancel AN 控制）。
		RHCombat->EnterHitReactionState();
	}

	if (UKnsAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		const FGameplayTag StaggeredTag = FGameplayTag::RequestGameplayTag(TEXT("Status.Staggered"), false);
		if (!ASC->HasMatchingGameplayTag(StaggeredTag))
		{
			ASC->AddLooseGameplayTag(StaggeredTag);
		}

		// 倒地（超大受击）：全程无敌直到起身 Notify（完全站起）。
		if (bKnockdown)
		{
			const FGameplayTag InvincibleTag = FGameplayTag::RequestGameplayTag(TEXT("Status.Invincible"), false);
			const FGameplayTag KnockedDownTag = FGameplayTag::RequestGameplayTag(TEXT("Status.KnockedDown"), false);
			if (InvincibleTag.IsValid())
			{
				ASC->AddLooseGameplayTag(InvincibleTag);
			}
			if (KnockedDownTag.IsValid())
			{
				ASC->AddLooseGameplayTag(KnockedDownTag);
			}
		}
	}

	if (bKnockdown)
	{
		// 超大受击无 F/L/R/B：受击瞬间面向攻击方向，蒙太奇整段播放（向后飞出）。
		if (AttackSource)
		{
			const FVector Dir = (AttackSource->GetActorLocation() - Character->GetActorLocation()).GetSafeNormal2D();
			if (!Dir.IsNearlyZero())
			{
				Character->SetActorRotation(Dir.Rotation());
			}
		}
		Character->PlayAnimMontage(Montage, 1.f);
	}
	else
	{
		const FName Section = UKnsHitReactionSettingsDataAsset::GetSectionNameForDirection(Direction);
		AnimInstance->Montage_Play(Montage, 1.f);
		AnimInstance->Montage_JumpToSection(Section, Montage);
	}
}

void UKnsCombatComponent::StopCurrentReactionMontage()
{
	if (!CurrentReactionMontage)
	{
		return;
	}

	UAnimMontage* OldMontage = CurrentReactionMontage;
	CurrentReactionMontage = nullptr;

	if (ABaseCharacter* Character = Cast<ABaseCharacter>(GetOwner()))
	{
		if (UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance())
		{
			AnimInstance->Montage_Stop(0.1f, OldMontage);
		}
	}
}

void UKnsCombatComponent::HandleReactionMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage != CurrentReactionMontage)
	{
		return;
	}

	CurrentReactionMontage = nullptr;

	if (URHCombatComponent* RHCombat = Cast<URHCombatComponent>(this))
	{
		RHCombat->ExitHitReactionState();
	}

	if (UKnsAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		const FGameplayTag StaggeredTag = FGameplayTag::RequestGameplayTag(TEXT("Status.Staggered"), false);
		ASC->RemoveLooseGameplayTag(StaggeredTag);

		const FGameplayTag InvincibleTag = FGameplayTag::RequestGameplayTag(TEXT("Status.Invincible"), false);
		const FGameplayTag KnockedDownTag = FGameplayTag::RequestGameplayTag(TEXT("Status.KnockedDown"), false);
		if (InvincibleTag.IsValid())
		{
			ASC->RemoveLooseGameplayTag(InvincibleTag);
		}
		if (KnockedDownTag.IsValid())
		{
			ASC->RemoveLooseGameplayTag(KnockedDownTag);
		}
	}

	OnHitReactionEnded.Broadcast();
}

void UKnsCombatComponent::BeginHitbox(FGameplayTag HitboxTag)
{
	ActiveHitboxTag = HitboxTag;
	HitActorsThisHitbox.Reset();

	// 调试：命中框开启期间让武器 HitboxBox 在游戏中可见（HiddenInGame + Visibility 双标志，照抄木桩 ProbeBox 显示方式），主/副框都显示。
	if (bShowHitboxDebug && SpawnedWeapon)
	{
		if (UBoxComponent* HitboxBox = SpawnedWeapon->GetHitboxBox())
		{
			HitboxBox->SetHiddenInGame(false);
			HitboxBox->SetVisibility(true, true);
		}
		if (SpawnedWeapon->HasSecondaryBlade())
		{
			if (UBoxComponent* SecondaryHitboxBox = SpawnedWeapon->GetSecondaryHitboxBox())
			{
				SecondaryHitboxBox->SetHiddenInGame(false);
				SecondaryHitboxBox->SetVisibility(true, true);
			}
		}
	}

	// 如果武器已经和敌人重叠，BeginOverlap 不会再次触发，需要主动查询一次（主/副刀 box 都补扫，副框仅双刀）。
	if (SpawnedWeapon)
	{
		if (UBoxComponent* HitboxBox = SpawnedWeapon->GetHitboxBox())
		{
			QueryHitboxOverlaps(HitboxBox);
		}
		if (SpawnedWeapon->HasSecondaryBlade())
		{
			if (UBoxComponent* SecondaryHitboxBox = SpawnedWeapon->GetSecondaryHitboxBox())
			{
				QueryHitboxOverlaps(SecondaryHitboxBox);
			}
		}
	}
}

void UKnsCombatComponent::QueryHitboxOverlaps(UBoxComponent* HitboxBox)
{
	if (!HitboxBox)
	{
		return;
	}

	TArray<UPrimitiveComponent*> OverlappingComponents;
	HitboxBox->GetOverlappingComponents(OverlappingComponents);

	for (UPrimitiveComponent* OverlappedComponent : OverlappingComponents)
	{
		if (OverlappedComponent && OverlappedComponent->GetOwner())
		{
			// 补扫路径没有真实 SweepResult：用 hitbox 盒子上离目标最近的点作为命中点。
			FHitResult Hit;
			FVector ClosestPoint;
			if (HitboxBox->GetClosestPointOnCollision(OverlappedComponent->GetOwner()->GetActorLocation(), ClosestPoint) >= 0.f)
			{
				Hit.ImpactPoint = ClosestPoint;
			}
			else
			{
				Hit.ImpactPoint = OverlappedComponent->GetComponentLocation();
			}
			HandleHitboxOverlap(HitboxBox, OverlappedComponent->GetOwner(), OverlappedComponent, 0, false, Hit);
		}
	}
}

void UKnsCombatComponent::EndHitbox(FGameplayTag HitboxTag)
{
	if (!ActiveHitboxTag.IsValid() || ActiveHitboxTag.MatchesTagExact(HitboxTag))
	{
		ActiveHitboxTag = FGameplayTag();
		HitActorsThisHitbox.Reset();

		// 调试：命中框关闭后恢复隐藏（仅显示期间改过才需要恢复，直接按开关判断即可）。
		if (bShowHitboxDebug && SpawnedWeapon)
		{
			if (UBoxComponent* HitboxBox = SpawnedWeapon->GetHitboxBox())
			{
				HitboxBox->SetHiddenInGame(true);
				HitboxBox->SetVisibility(false, true);
			}
			if (SpawnedWeapon->HasSecondaryBlade())
			{
				if (UBoxComponent* SecondaryHitboxBox = SpawnedWeapon->GetSecondaryHitboxBox())
				{
					SecondaryHitboxBox->SetHiddenInGame(true);
					SecondaryHitboxBox->SetVisibility(false, true);
				}
			}
		}
	}
}

void UKnsCombatComponent::ResolveWeaponClash(AActor* OtherWeapon)
{
	if (!OtherWeapon)
	{
		return;
	}
	// 对方命中框对这把武器完成结算：记录并关闭当前命中框（不会再打到身体）。
	HitActorsThisHitbox.Add(OtherWeapon);
	EndHitbox(ActiveHitboxTag);
}

bool UKnsCombatComponent::ReportHit(AActor* TargetActor, FVector HitLocation, FGameplayTag HitboxTag)
{
	if (!ActiveHitboxTag.IsValid() || !ActiveHitboxTag.MatchesTagExact(HitboxTag))
	{
		return false;
	}

	if (!TargetActor || HitActorsThisHitbox.Contains(TargetActor))
	{
		return false;
	}

	UKnsMoveDefinition* Move = CombatContext ? CombatContext->GetCurrentMove() : nullptr;
	URHMoveDefinition* RHMove = nullptr;
	if (!Move && WeaponDefinition)
	{
		RHMove = WeaponDefinition->DefaultMoveDefinition.LoadSynchronous();
	}

	if (!Move && !RHMove)
	{
		if (WeaponDefinition)
		{
			HitActorsThisHitbox.Add(TargetActor);

			FRHHitData HitData;
			HitData.Damage = WeaponDefinition->DefaultDamage;
			HitData.ResonanceDamage = WeaponDefinition->DefaultResonanceDamage;
			HitData.Source = GetOwner();
			HitData.HitLocation = HitLocation;
			ApplyHitToTarget(TargetActor, HitData);
			UE_LOG(LogTemp, Warning, TEXT("RH ReportHit used weapon default damage"));
			return true;
		}

		UE_LOG(LogTemp, Warning, TEXT("RH ReportHit rejected: no current move and no weapon fallback"));
		return false;
	}

	HitActorsThisHitbox.Add(TargetActor);

	FRHHitData HitData;
	if (RHMove)
	{
		HitData.Damage = RHMove->ActionValue;
		// 普通连段未配共振伤害时回落到武器默认值，避免“掉血但共振不涨”。
		const float MoveResonance = RHMove->ResonanceDamage;
		HitData.ResonanceDamage = MoveResonance > 0.f
			? MoveResonance
			: (WeaponDefinition ? WeaponDefinition->DefaultResonanceDamage : 0.f);
		HitData.CounterBarDamage = RHMove->CounterBarDamage;
	}
	else
	{
		HitData.Damage = Move->ActionValue;
		HitData.ResonanceDamage = Move->PoiseDamage;
		HitData.CounterBarDamage = Move->CounterBarDamage;
	}
	HitData.Source = GetOwner();
	HitData.HitLocation = HitLocation;
	const FGameplayTag AttackTag = RHMove ? RHMove->AttackTag : Move->AttackTag;
	if (AttackTag.IsValid())
	{
		HitData.Tags.AddTag(AttackTag);
	}
	ApplyHitToTarget(TargetActor, HitData);

	if (RHMove)
	{
		PlayHitStop(RHMove->HitStopLevel);
	}
	else
	{
		PlayHitStop(Move->HitStopLevel);
		OnHitLanded.Broadcast(Move, TargetActor, HitLocation, HitboxTag);
	}

	if (UWorld* World = GetWorld())
	{
		if (UKnsCombatDebugSubsystem* DebugSubsystem = World->GetSubsystem<UKnsCombatDebugSubsystem>())
		{
			DebugSubsystem->LogEvent(TEXT("HitLanded"), FColor::Green, FString::Printf(TEXT("Target=%s Move=%s"), *TargetActor->GetName(), *Move->GetName()));
		}
	}

	return true;
}

bool UKnsCombatComponent::ApplyHitToTarget(AActor* Target, const FRHHitData& InHitData)
{
	if (!Target)
	{
		return false;
	}

	FRHHitData HitData = InHitData;

	const bool bActionHit = CurrentActionState.Action != nullptr;
	CurrentHitOnomPolarity = ERHOnomPolarity::None;
	URHOnomComponent* Onom = GetOnomComponent();
	FRHOnomSourceRule PendingOnomRule;
	bool bAddOnomOnLand = false;
	// Resonance 命中模式：与音形一样延迟到命中落地才入账（命中被忽略——无敌帧/弹开——时不涨共鸣也不播特效）。
	ERHOnomPolarity PendingResonancePolarity = ERHOnomPolarity::Neutral;
	float PendingResonanceDecayRate = 1.f;
	bool bGainResonanceOnLand = false;

	// 命中 Onom 结算统一出口：敌人覆写 → 极性映射（供反馈选档）→ 挂 pending（实际入账等命中落地）。
	auto ResolveAndQueueOnom = [&](FRHOnomSourceRule Rule)
	{
		if (ARHEnemyBase* Enemy = Cast<ARHEnemyBase>(Target))
		{
			if (Enemy->OnomReactionOverride.bSuppressOnomGain)
			{
				return;
			}
			if (Enemy->OnomReactionOverride.bOverrideOnomHitRule)
			{
				Rule = Enemy->OnomReactionOverride.OverrideRule;
			}
		}
		switch (Rule.Type)
		{
		case ERHOnomValue::Positive: CurrentHitOnomPolarity = ERHOnomPolarity::Major; break;
		case ERHOnomValue::Negative: CurrentHitOnomPolarity = ERHOnomPolarity::Minor; break;
		case ERHOnomValue::Broken: CurrentHitOnomPolarity = ERHOnomPolarity::Neutral; break;
		case ERHOnomValue::Neutral: CurrentHitOnomPolarity = ERHOnomPolarity::Neutral; break;
		default: CurrentHitOnomPolarity = ERHOnomPolarity::None; break;
		}
		PendingOnomRule = Rule;
		bAddOnomOnLand = true;
	};

	if (Onom && WeaponDefinition)
	{
		// ① 命中数据显式指定（Blitz 选极性）：最高优先级——强制产音形，绕过战技限制与武器 HitGainMode。
		if (HitData.bOverrideOnomRule)
		{
			ResolveAndQueueOnom(HitData.OverrideOnomRule);
		}
		// ② 命中获得模式：Resonance = 攒共鸣（层+1，衰减速率按武器 DA 配置），不产生音形。
		// 反馈极性照常设置（按主音规则类型），与音形模式一致——命中特效不再落到 None 档。
		// 战技命中不触发 hit rule：不攒共鸣（与 ③ 音形模式一样排除战技）。
		else if (!bActionHit && WeaponDefinition->HitGainMode == ERHOnomHitGainMode::Resonance)
		{
			ERHOnomPolarity GainPolarity = ERHOnomPolarity::Neutral;
			switch (WeaponDefinition->AttackHitRule.Type)
			{
			case ERHOnomValue::Positive: GainPolarity = ERHOnomPolarity::Major; break;
			case ERHOnomValue::Negative: GainPolarity = ERHOnomPolarity::Minor; break;
			default: break; // Neutral/Broken/None → 平调
			}
			CurrentHitOnomPolarity = GainPolarity;
			PendingResonancePolarity = GainPolarity;
			PendingResonanceDecayRate = WeaponDefinition->ResonanceDecayRate;
			bGainResonanceOnLand = true;
		}
		// ③ 默认：武器 AttackHitRule 兜底（普攻 Move 可自选 bOverrideHitOnom）。
		// 战技命中不触发 hit rule：不获得音形（与 ② 共鸣模式一样排除战技），资源走向由武器 HitGainMode 统一决定。
		else if (!bActionHit)
		{
			FRHOnomSourceRule Rule = WeaponDefinition->AttackHitRule;
			FRHOnomSourceRule MoveRule;
			if (GetHitOnomRuleOverride(MoveRule))
			{
				Rule = MoveRule;
			}
			ResolveAndQueueOnom(Rule);
		}
	}

	if (bActionHit)
	{
		HitData.bIsSkill = true;
		if (CurrentActionState.Resolved.Damage > 0)
		{
			HitData.Damage = CurrentActionState.Resolved.Damage;
		}
		if (CurrentActionState.Resolved.ResonanceDamage > 0)
		{
			HitData.ResonanceDamage = CurrentActionState.Resolved.ResonanceDamage;
		}

		// 首次命中：应用共鸣增幅与打折，并记录轰鸣格数。打折系数统一作用于伤害/共鸣伤害/充能获取。
		if (PendingActionCast.Action)
		{
			const float BaseDamage = HitData.Damage;
			const float BaseResonance = HitData.ResonanceDamage;
			HitData.Damage *= PendingActionCast.Amplification.DamageMultiplier * PendingActionCast.DiscountMultiplier;
			HitData.ResonanceDamage *= PendingActionCast.Amplification.ResonanceDamageMultiplier * PendingActionCast.DiscountMultiplier;
			HitData.ChargeGaugeCount = PendingActionCast.Consumption.HandConsumed
				+ (PendingActionCast.Consumption.bResonanceConsumed ? PendingActionCast.Consumption.ResonanceLevel : 0);
			HitData.ChargeMultiplier = PendingActionCast.Amplification.ChargeMultiplier * PendingActionCast.DiscountMultiplier;
			UE_LOG(LogTemp, Warning, TEXT("RH Hit Amplify: dmg %.1f -> %.1f (x%.2f amp x%.2f discount), res %.1f -> %.1f (x%.2f x%.2f), charge x%.2f"),
				BaseDamage, HitData.Damage,
				PendingActionCast.Amplification.DamageMultiplier, PendingActionCast.DiscountMultiplier,
				BaseResonance, HitData.ResonanceDamage,
				PendingActionCast.Amplification.ResonanceDamageMultiplier, PendingActionCast.DiscountMultiplier,
				HitData.ChargeMultiplier);
		}
	}

	UKnsAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC || !ASC->ApplyHitToActor(Target, HitData))
	{
		// 命中被忽略（无敌帧/敌人弹开等）：不结算音形/轰鸣，也不广播命中。
		return false;
	}

	// 命中落地：真正入账 Onom。
	if (bAddOnomOnLand && Onom)
	{
		Onom->AddOnom(PendingOnomRule, Target);
	}

	// 命中落地：共鸣入账（Resonance 命中模式）。
	if (bGainResonanceOnLand && Onom)
	{
		Onom->AddResonanceLayer(PendingResonancePolarity, PendingResonanceDecayRate);
	}

	if (PendingActionCast.Action && Onom)
	{
		const float ChargePerOnom = Onom->Settings ? Onom->Settings->ChargePercentPerOnom : 5.f;
		Onom->AddCharge(HitData.ChargeGaugeCount * ChargePerOnom * HitData.ChargeMultiplier);
		PendingActionCast = FRHOnomActionCastContext();
	}

	OnHitApplied.Broadcast(Target, HitData);
	UE_LOG(LogTemp, Warning, TEXT("RH Hit: %s Damage=%.1f Resonance=%.1f"),
		Target ? *Target->GetName() : TEXT("None"), HitData.Damage, HitData.ResonanceDamage);
	return true;
}

void UKnsCombatComponent::HandleNormalGuardHit()
{
	ResolveGuardHit(false);
}

void UKnsCombatComponent::HandlePerfectGuardHit()
{
	ResolveGuardHit(true);
}

void UKnsCombatComponent::SetGuarding(bool bInGuarding)
{
	if (bGuarding == bInGuarding)
	{
		return;
	}

	bGuarding = bInGuarding;

	UKnsAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	const FGameplayTag GuardingTag = FGameplayTag::RequestGameplayTag(TEXT("Status.Guarding"), false);
	if (ASC && GuardingTag.IsValid())
	{
		if (bGuarding)
		{
			ASC->AddLooseGameplayTag(GuardingTag);
		}
		else
		{
			ASC->RemoveLooseGameplayTag(GuardingTag);
		}
	}

	// 防御态减速（200），结束/被打断恢复默认速度（600）。
	if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		if (UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement())
		{
			MoveComp->MaxWalkSpeed = bGuarding ? GuardWalkSpeed : DefaultMaxWalkSpeed;
		}
	}
}

bool UKnsCombatComponent::IsGuarding() const
{
	return bGuarding;
}

bool UKnsCombatComponent::IsReactionAnimationActive() const
{
	// 1) 状态指针仍在 → 硬直中。
	if (CurrentReactionMontage)
	{
		return true;
	}
	// 2) 指针已被清空（被替换/Stop）但动画实例上仍有蒙太奇在播：
	//    旧受击蒙太奇 blend out（Montage_Stop 的 0.1s 淡出）或替换间隙仍算硬直，
	//    否则 WaitRecover 会提前结束、敌人立刻反击。
	if (const ABaseCharacter* Character = Cast<ABaseCharacter>(GetOwner()))
	{
		if (const UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance())
		{
			return AnimInstance->IsAnyMontagePlaying();
		}
	}
	return false;
}

void UKnsCombatComponent::PlayDefensiveBreakReaction(AActor* AttackSource)
{
	UKnsHitReactionSettingsDataAsset* ReactionSettings = GetHitReactionData();
	UAnimMontage* Montage = ReactionSettings ? ReactionSettings->GetDefensiveBreakMontage() : nullptr;
	if (!Montage)
	{
		// 兜底：普通中受击。
		HandleIncomingHitFrom(AttackSource ? AttackSource : GetOwner(), 2);
		return;
	}

	ABaseCharacter* Character = Cast<ABaseCharacter>(GetOwner());
	UAnimInstance* AnimInstance = Character ? Character->GetMesh()->GetAnimInstance() : nullptr;
	if (!Character || !AnimInstance)
	{
		return;
	}

	StopCurrentReactionMontage();
	CurrentReactionMontage = Montage;

	// 打断当前动作（弹开/破防）。
	if (URHCombatComponent* RHCombat = Cast<URHCombatComponent>(this))
	{
		RHCombat->CancelAction();
		RHCombat->EnterHitReactionState();
	}

	if (UKnsAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		const FGameplayTag StaggeredTag = FGameplayTag::RequestGameplayTag(TEXT("Status.Staggered"), false);
		if (!ASC->HasMatchingGameplayTag(StaggeredTag))
		{
			ASC->AddLooseGameplayTag(StaggeredTag);
		}
	}

	const FVector Point = AttackSource ? AttackSource->GetActorLocation() : Character->GetActorLocation();
	const EKnsHitDirection Direction = ComputeHitDirectionFromPoint(Point, Character);
	const FName Section = UKnsHitReactionSettingsDataAsset::GetSectionNameForDirection(Direction);
	AnimInstance->Montage_Play(Montage, 1.f);
	AnimInstance->Montage_JumpToSection(Section, Montage);
}

bool UKnsCombatComponent::IsPerfectGuardWindowActive() const
{
	return bPerfectGuardWindowActive;
}

void UKnsCombatComponent::SetInvincible(bool bInvincible)
{
	if (UKnsAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		const FGameplayTag InvincibleTag = FGameplayTag::RequestGameplayTag(TEXT("Status.Invincible"), false);
		if (!InvincibleTag.IsValid())
		{
			return;
		}
		if (bInvincible)
		{
			ASC->AddLooseGameplayTag(InvincibleTag);
		}
		else
		{
			ASC->RemoveLooseGameplayTag(InvincibleTag);
		}
	}
}

ERHOnomPolarity UKnsCombatComponent::GetCurrentCastPolarity() const
{
	return CurrentCastPolarity;
}

ERHOnomPolarity UKnsCombatComponent::GetCurrentHitOnomPolarity() const
{
	return CurrentHitOnomPolarity;
}

int32 UKnsCombatComponent::GetCurrentActionConsumedCount() const
{
	return CurrentActionState.ConsumedCount;
}

int32 UKnsCombatComponent::GetCurrentActionAbsoluteSum() const
{
	return CurrentActionState.ConsumedAbsoluteSum;
}

void UKnsCombatComponent::OpenPerfectGuardWindow(float Seconds)
{
	if (Seconds <= 0.f)
	{
		bPerfectGuardWindowActive = false;
		ApplyParryOverlay(false);
		return;
	}

	bPerfectGuardWindowActive = true;
	ApplyParryOverlay(true);
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			PerfectGuardTimerHandle,
			this,
			&UKnsCombatComponent::HandlePerfectGuardWindowElapsed,
			Seconds,
			false);
	}
}

void UKnsCombatComponent::ClosePerfectGuardWindow()
{
	bPerfectGuardWindowActive = false;
	ApplyParryOverlay(false);
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PerfectGuardTimerHandle);
	}
}

void UKnsCombatComponent::HandlePerfectGuardWindowElapsed()
{
	bPerfectGuardWindowActive = false;
	ApplyParryOverlay(false);
}

void UKnsCombatComponent::ApplyParryOverlay(bool bActive)
{
	if (!ParryOverlayMaterial)
	{
		return;
	}

	if (ABaseCharacter* Character = Cast<ABaseCharacter>(GetOwner()))
	{
		if (USkeletalMeshComponent* Mesh = Character->GetMesh())
		{
			Mesh->SetOverlayMaterial(bActive ? ParryOverlayMaterial : nullptr);
		}
	}
}

ERHOnomGuardOutcome UKnsCombatComponent::ResolveGuardHit(bool bPerfect)
{
	URHOnomComponent* Onom = GetOnomComponent();
	if (!Onom)
	{
		return ERHOnomGuardOutcome::None;
	}

	if (bPerfect)
	{
		if (WeaponDefinition)
		{
			// 完美防御获得模式：Onom=获得次音规则音形；Resonance=共鸣层+1（类型按次音规则，衰减按武器 DA）。
			if (WeaponDefinition->PerfectGuardGainMode == ERHOnomHitGainMode::Resonance)
			{
				Onom->AddResonanceLayer(
					URHOnomComponent::GetPolarityFromValue(WeaponDefinition->PerfectGuardHitRule.Type),
					WeaponDefinition->ResonanceDecayRate);
			}
			else
			{
				Onom->AddOnom(WeaponDefinition->PerfectGuardHitRule, GetOwner());
			}
		}
		UE_LOG(LogTemp, Warning, TEXT("RH Perfect Guard: granted"));
		return ERHOnomGuardOutcome::Added;
	}

	return Onom->AddBrokenOnom(GetOwner());
}

void UKnsCombatComponent::ApplyBigBreak(AActor* AttackSource)
{
	URHOnomComponent* Onom = GetOnomComponent();
	const float StunSeconds = 2.f;
	PlayDefensiveCameraShake(ERHDefensiveShakeType::BigBreak);

	if (UKnsAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		// 破防不扣血：算玩家成功防下，只吃硬直。
		const FGameplayTag StaggeredTag = FGameplayTag::RequestGameplayTag(TEXT("Status.Staggered"), false);
		if (StaggeredTag.IsValid())
		{
			ASC->AddLooseGameplayTag(StaggeredTag);
		}
	}

	// 防御大受击动画（防御破防蒙太奇）；内部会打断当前动作（含退出 block）。
	PlayDefensiveBreakReaction(AttackSource);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			BigBreakTimerHandle,
			this,
			&UKnsCombatComponent::HandleBigBreakEnded,
			StunSeconds,
			false);
	}

	UE_LOG(LogTemp, Warning, TEXT("RH Big Break: no damage, stun %.1fs"), StunSeconds);
}

void UKnsCombatComponent::HandleBigBreakEnded()
{
	if (UKnsAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		const FGameplayTag StaggeredTag = FGameplayTag::RequestGameplayTag(TEXT("Status.Staggered"), false);
		if (StaggeredTag.IsValid())
		{
			ASC->RemoveLooseGameplayTag(StaggeredTag);
		}
	}
}

void UKnsCombatComponent::HandleDamageTaken(AActor* Instigator)
{
	URHOnomComponent* Onom = GetOnomComponent();
	if (!Onom)
	{
		return;
	}

	if (Onom->CoreDefinition && Onom->CoreDefinition->DamageTakenEffects.Num() > 0)
	{
		// 受击效果数组优先（替代写死的“清空 + 共鸣-2s”）。
		ApplyOnomEffects(Onom->CoreDefinition->DamageTakenEffects);
	}
	else
	{
		// 回落：旧逻辑（清空/规则结算 + 共鸣-2s）。
		Onom->ApplyDamageTakenRule(Instigator);
	}
	UE_LOG(LogTemp, Warning, TEXT("RH Damage Taken: Onom penalty applied"));
}

void UKnsCombatComponent::HandleDodgeSuccess()
{
	URHOnomComponent* Onom = GetOnomComponent();
	if (!Onom)
	{
		return;
	}

	if (Onom->DodgeCoreDefinition && Onom->DodgeCoreDefinition->DodgeEffects.Num() > 0)
	{
		// Dodge Core DA 效果数组优先（替代写死的共鸣 +X 秒）。
		ApplyOnomEffects(Onom->DodgeCoreDefinition->DodgeEffects);
	}
	else
	{
		// 回落：未配 Dodge Core DA 时的旧行为（共鸣 +1.5s，数值已由 Dodge Core DA 接管）。
		Onom->AddResonanceTime(1.5f);
	}
	UE_LOG(LogTemp, Warning, TEXT("RH Dodge success effects applied"));
}

void UKnsCombatComponent::ApplyOnomEffects(const TArray<FRHOnomEffect>& Effects)
{
	URHOnomComponent* Onom = GetOnomComponent();
	if (!Onom)
	{
		return;
	}
	AActor* Owner = GetOwner();

	for (const FRHOnomEffect& Effect : Effects)
	{
		switch (Effect.Type)
		{
		case ERHOnomEffectType::AddResonanceTime:
			if (Effect.Amount > 0.f)
			{
				Onom->AddResonanceTime(Effect.Amount);
			}
			else if (Effect.Amount < 0.f)
			{
				Onom->SubtractResonanceTime(-Effect.Amount);
			}
			break;

		case ERHOnomEffectType::ModifyOnomCount:
			if (Effect.Count > 0)
			{
				FRHOnomSourceRule Rule;
				Rule.bEnabled = true;
				Rule.Mode = ERHOnomRuleMode::Add;
				Rule.Type = Effect.ValueType;
				Rule.Count = Effect.Count;
				Onom->AddOnom(Rule, Owner);
			}
			else if (Effect.Count < 0)
			{
				// 减少：移除任意手牌（非灰优先，见 RemoveOnom）。
				Onom->RemoveOnom(-Effect.Count);
			}
			break;

		case ERHOnomEffectType::TriggerJustLoad:
			ExecuteJustLoad(Effect.bPlayVFX);
			break;

		case ERHOnomEffectType::TriggerParry:
			TriggerParryEffect();
			break;

		default:
			break;
		}
	}
}

void UKnsCombatComponent::ExecuteJustLoad(bool bPlayVFX)
{
	URHOnomComponent* Onom = GetOnomComponent();
	if (!Onom)
	{
		return;
	}

	if (Onom->HasGreyOnom())
	{
		// 有灰色：抛弹（清空手牌含灰色），播灰色音效。
		Onom->ClearHandOnom();
		Onom->PlayGainSound(ERHOnomValue::Broken);
		UE_LOG(LogTemp, Warning, TEXT("RH JustLoad -> Toss: hand cleared"));
	}
	else if (Onom->GetNonGreyOnomCount() > 0)
	{
		// 无灰色：装填（非灰手牌全部存入共鸣，内部播装填音效）。
		Onom->TryStoreToResonance();
	}
	// 既无灰色也无非灰手牌：无可装/可抛，忽略。
}

void UKnsCombatComponent::TriggerParryEffect()
{
	// 基类空实现：敌人等无弹反效果；玩家组件覆写为 NotifyParrySuccess。
}

bool UKnsCombatComponent::TryStartAction(URHOnomActionDefinition* Action)
{
	URHOnomComponent* Onom = GetOnomComponent();
	if (!Onom || !Action)
	{
		return false;
	}

	// 用“实际会消耗的 RequiredCount 个”模拟数据匹配（预览极性=消耗后极性）。
	const FRHOnomConsumptionData Preview = Onom->SimulateConsumeOnom(Action->RequiredCount);
	if (!Action->MatchesConsumption(Preview))
	{
		UE_LOG(LogTemp, Warning, TEXT("RH Action rejected: %s"), *Action->ActionId.ToString());
		return false;
	}

	const FRHOnomConsumptionData Consumption = Onom->ConsumeOnom(Action->RequiredCount);
	if (Consumption.ConsumedCount <= 0)
	{
		return false;
	}

	FRHOnomActionCastContext Context;
	Context.Action = Action;
	Context.Consumption = Consumption;
	Context.Amplification = Onom->ComputeAmplification(Consumption, WeaponDefinition);
	Context.DiscountMultiplier = Action->GetDiscountMultiplier(Consumption);
	PendingActionCast = Context;

	OnActionCast.Broadcast(Action, Consumption);
	UE_LOG(LogTemp, Warning, TEXT("RH Action started: %s Count=%d Hand=%d Resonance=%d Abs=%d"),
		*Action->ActionId.ToString(), Consumption.ConsumedCount, Consumption.HandConsumed, Consumption.ResonanceLevel, Consumption.AbsoluteSum);
	return true;
}

TArray<URHOnomActionDefinition*> UKnsCombatComponent::GetEligibleActions(const TArray<URHOnomActionDefinition*>& Actions)
{
	TArray<URHOnomActionDefinition*> Eligible;
	URHOnomComponent* Onom = GetOnomComponent();
	if (!Onom)
	{
		return Eligible;
	}

	// 每个动作按自己的 RequiredCount 模拟实际消耗再匹配，UI 可选列表与真实释放一致。
	for (URHOnomActionDefinition* Action : Actions)
	{
		if (Action && Action->MatchesConsumption(Onom->SimulateConsumeOnom(Action->RequiredCount)))
		{
			Eligible.Add(Action);
		}
	}

	Eligible.Sort([](const URHOnomActionDefinition& A, const URHOnomActionDefinition& B)
	{
		return A.Priority > B.Priority;
	});

	return Eligible;
}

void UKnsCombatComponent::HandleHitboxOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherActor || OtherActor == GetOwner() || OtherActor == SpawnedWeapon)
	{
		return;
	}

	// 目前只检测角色类目标，避免碰到武器/场景物件也触发卡肉
	if (!Cast<ABaseCharacter>(OtherActor))
	{
		return;
	}
	// 本段命中框已对该目标结算过（普通命中或相杀）：跳过，防止命中+相杀重复给 Onom。
	if (HasResolvedHitboxTarget(OtherActor))
	{
		return;
	}

	// 非 sweep 的重叠事件 ImpactPoint 恒为 0：回落到目标组件位置，保证命中点有效。
	FVector HitLocation = SweepResult.ImpactPoint;
	if (HitLocation.IsNearlyZero())
	{
		// 优先取 hitbox 碰撞盒上离目标最近的点（非 sweep 没有真实碰撞点）。
		FVector ClosestPoint;
		if (OverlappedComponent && OverlappedComponent->GetClosestPointOnCollision(OtherActor->GetActorLocation(), ClosestPoint) >= 0.f)
		{
			HitLocation = ClosestPoint;
		}
		else
		{
			HitLocation = OtherComp ? OtherComp->GetComponentLocation() : (OtherActor ? OtherActor->GetActorLocation() : FVector::ZeroVector);
		}
	}
	// 相杀判定：身体接触瞬间检查对方窗口状态（不再依赖 hitbox 相撞/延迟）。
	if (URHCombatComponent* RHCombat = Cast<URHCombatComponent>(this))
	{
		// 玩家攻击方：clash 开着且敌人命中框也开着（武器 hitbox 或木桩判定框）→ 相杀。
		if (RHCombat->IsClashWindowOpen())
		{
			if (UKnsCombatComponent* EnemyCombat = OtherActor->FindComponentByClass<UKnsCombatComponent>())
			{
				if (EnemyCombat->IsClashableHitboxActive())
				{
					// 木桩等无武器的实体直接以自身为相杀对象（HandleWeaponClash 内支持 Owner 空回落）。
					AActor* ClashWeapon = EnemyCombat->SpawnedWeapon ? EnemyCombat->SpawnedWeapon : OtherActor;
					RHCombat->HandleWeaponClash(ClashWeapon, HitLocation);
					return;
				}
			}
		}
	}
	else if (IsHitboxActive())
	{
		// 非玩家攻击方（敌人）命中玩家身体：目标 clash 开着 → 相杀。
		if (URHCombatComponent* TargetRHCombat = OtherActor->FindComponentByClass<URHCombatComponent>())
		{
			if (TargetRHCombat->IsClashWindowOpen() && SpawnedWeapon)
			{
				TargetRHCombat->HandleWeaponClash(SpawnedWeapon, HitLocation);
				return;
			}
		}
	}
	ReportHit(OtherActor, HitLocation, ActiveHitboxTag);
}

void UKnsCombatComponent::HandleBodyHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (!bTouchConditionEnabled || !OtherActor || OtherActor == GetOwner() || !Cast<ABaseCharacter>(OtherActor))
	{
		return;
	}

	if (AActor* Owner = GetOwner())
	{
		if (CombatContext)
		{
			if (UWorld* World = GetWorld())
			{
				if (UKnsCombatDebugSubsystem* DebugSubsystem = World->GetSubsystem<UKnsCombatDebugSubsystem>())
				{
					DebugSubsystem->LogEvent(TEXT("BodyTouch"), FColor::Cyan, OtherActor->GetName());
				}
			}
			CombatContext->RequestHitConditionTag(FGameplayTag::RequestGameplayTag(TEXT("Combo.Condition.Touch"), false));
		}
	}
}

void UKnsCombatComponent::PlayDefensiveCameraShake(ERHDefensiveShakeType Type)
{
	if (!DefensiveCameraShakeDefinition)
	{
		return;
	}

	FRHDefensiveShakeEntry Entry;
	if (!DefensiveCameraShakeDefinition->GetShake(Type, Entry) || !Entry.CameraShake)
	{
		return;
	}

	ABaseCharacter* Character = Cast<ABaseCharacter>(GetOwner());
	if (!Character)
	{
		return;
	}

	if (APlayerController* PlayerController = Cast<APlayerController>(Character->GetController()))
	{
		PlayerController->ClientStartCameraShake(Entry.CameraShake, Entry.ShakeScale);
	}
}

void UKnsCombatComponent::PlayCastFlash()
{
	if (!CastFlashComponent)
	{
		return;
	}
	if (SpecialCastFlash)
	{
		CastFlashComponent->SetAsset(SpecialCastFlash);
	}
	CastFlashComponent->Activate(true); // 每次施放重播
}

void UKnsCombatComponent::PlayHitStop(int32 Level)
{
	if (Level <= 0 || !HitStopSettings)
	{
		return;
	}

	FHitStopLevelSettings Settings;
	if (!HitStopSettings->GetSettings(Level, Settings) || Settings.Duration <= 0.f)
	{
		return;
	}

	ABaseCharacter* Character = Cast<ABaseCharacter>(GetOwner());
	if (!Character)
	{
		return;
	}

	UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance();
	UAnimMontage* ActiveMontage = AnimInstance ? AnimInstance->GetCurrentActiveMontage() : nullptr;
	if (!AnimInstance || !ActiveMontage)
	{
		return;
	}

	if (!GetWorld()->GetTimerManager().IsTimerActive(HitStopTimerHandle))
	{
		OriginalMontagePlayRate = AnimInstance->Montage_GetPlayRate(ActiveMontage);
	}

	AnimInstance->Montage_SetPlayRate(ActiveMontage, Settings.MontagePlayRate);

	if (Settings.CameraShake)
	{
		if (APlayerController* PlayerController = Cast<APlayerController>(Character->GetController()))
		{
			PlayerController->ClientStartCameraShake(Settings.CameraShake, Settings.CameraShakeStrength);
		}
	}

	// 手柄震动：攻击方触发（玩家命中敌人震自己手柄；敌人命中玩家时 GetController 是 AI 控制器，回落震玩家手柄）。
	if (Settings.RumbleIntensity > 0.f)
	{
		APlayerController* PlayerController = Cast<APlayerController>(Character->GetController());
		if (!PlayerController)
		{
			PlayerController = GetWorld()->GetFirstPlayerController();
		}
		if (PlayerController)
		{
			PlayControllerRumble(PlayerController, Level, Settings.Duration);
		}
	}

	if (Settings.CameraRig && Settings.CameraRigDuration > 0.f)
	{
		if (UGameplayCameraComponent* GameplayCamera = Character->FindComponentByClass<UGameplayCameraComponent>())
		{
			const FCameraRigInstanceID RigID = GameplayCamera->ActivatePersistentVisualCameraRig(Settings.CameraRig);
			TWeakObjectPtr<UGameplayCameraComponent> WeakCamera(GameplayCamera);
			FTimerHandle CameraRigTimer;
			GetWorld()->GetTimerManager().SetTimer(
				CameraRigTimer,
				[WeakCamera, RigID]()
				{
					if (UGameplayCameraComponent* Camera = WeakCamera.Get())
					{
						Camera->DeactivateCameraRig(RigID);
					}
				},
				Settings.CameraRigDuration,
				false);
		}
	}

	GetWorld()->GetTimerManager().SetTimer(
		HitStopTimerHandle,
		FTimerDelegate::CreateUObject(this, &UKnsCombatComponent::RestoreMontagePlayRate, AnimInstance, ActiveMontage),
		Settings.Duration,
		false);

	OnHitStopTriggered.Broadcast(Level, Settings.Duration, Settings.CameraShakeStrength);

	if (UWorld* World = GetWorld())
	{
		if (UKnsCombatDebugSubsystem* DebugSubsystem = World->GetSubsystem<UKnsCombatDebugSubsystem>())
		{
			DebugSubsystem->LogEvent(TEXT("HitStop"), FColor::Blue, FString::Printf(TEXT("Level=%d Duration=%.2f"), Level, Settings.Duration));
		}
	}
}

void UKnsCombatComponent::RestoreMontagePlayRate(UAnimInstance* AnimInstance, UAnimMontage* Montage)
{
	if (AnimInstance && Montage)
	{
		AnimInstance->Montage_SetPlayRate(Montage, OriginalMontagePlayRate);
	}
}

void UKnsCombatComponent::PlayControllerRumble(APlayerController* PlayerController, int32 HitStopLevel, float DefaultDuration)
{
	if (!PlayerController || HitStopLevel <= 0 || !HitStopSettings)
	{
		return;
	}

	FHitStopLevelSettings Settings;
	if (!HitStopSettings->GetSettings(HitStopLevel, Settings) || Settings.RumbleIntensity <= 0.f)
	{
		return;
	}

	const float RumbleDuration = Settings.RumbleDuration > 0.f ? Settings.RumbleDuration : DefaultDuration;
	PlayerController->PlayDynamicForceFeedback(
		Settings.RumbleIntensity,
		RumbleDuration,
		Settings.bAffectsLeftLarge,
		Settings.bAffectsLeftSmall,
		Settings.bAffectsRightLarge,
		Settings.bAffectsRightSmall);
}

UKnsAbilitySystemComponent* UKnsCombatComponent::GetAbilitySystemComponent() const
{
	if (AActor* Owner = GetOwner())
	{
		if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Owner))
		{
			return Cast<UKnsAbilitySystemComponent>(ASI->GetAbilitySystemComponent());
		}
	}

	return nullptr;
}

URHOnomComponent* UKnsCombatComponent::GetOnomComponent() const
{
	if (CachedOnomComponent)
	{
		return CachedOnomComponent;
	}

	if (AActor* Owner = GetOwner())
	{
		return Owner->FindComponentByClass<URHOnomComponent>();
	}

	return nullptr;
}

UKnsHitReactionSettingsDataAsset* UKnsCombatComponent::GetHitReactionData() const
{
	return WeaponDefinition ? WeaponDefinition->HitReaction : nullptr;
}

void UKnsCombatComponent::ActivateCastTrail()
{
	if (SpawnedWeapon)
	{
		SpawnedWeapon->ActivateWeaponTrail();
	}
}

void UKnsCombatComponent::DeactivateCastTrail()
{
	if (SpawnedWeapon)
	{
		SpawnedWeapon->DeactivateWeaponTrail();
	}
}
