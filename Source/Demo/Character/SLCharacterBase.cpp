#include "SLCharacterBase.h"

#include "Demo/AI/RHEnemyAIComponent.h"
#include "Demo/AI/RHEnemyCombatComponent.h"
#include "Demo/Character/RHEnemyBase.h"
#include "Animation/AnimMontage.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/PlayerController.h"

namespace
{
	constexpr float SLQuickTurnAngle = 135.f;  // |input-vs-facing angle| >= this → opposite
}

ASLCharacterBase::ASLCharacterBase()
{
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Overlap);
	GetCapsuleComponent()->SetGenerateOverlapEvents(true);

	SLAbilitySystemComponent = CreateDefaultSubobject<UKnsAbilitySystemComponent>(TEXT("SL_ASC"));
	SLCommonAttributeSet = CreateDefaultSubobject<UKnsCommonAttributeSet>(TEXT("SL_CommonAS"));
	RHCombatComponent = CreateDefaultSubobject<URHCombatComponent>(TEXT("RH_Combat"));
	SLTargetLockComponent = CreateDefaultSubobject<UKnsTargetLockComponent>(TEXT("SL_TargetLock"));
	RHOnomComponent = CreateDefaultSubobject<URHOnomComponent>(TEXT("RH_Onom"));
	RHEquipComponent = CreateDefaultSubobject<URHEquipComponent>(TEXT("RH_Equip"));
}

void ASLCharacterBase::BeginPlay()
{
	Super::BeginPlay();
	SLAbilitySystemComponent->InitAbilityActorInfo(this, this);
	RHCombatComponent->InitializeAfterAbilitySystem();

	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		AnimInstance->OnMontageEnded.AddDynamic(this, &ASLCharacterBase::HandleExecutionMontageEnded);
	}
	if (SLTargetLockComponent)
	{
		SLTargetLockComponent->OnLockAcquired.AddDynamic(this, &ASLCharacterBase::HandleLockIndicatorChanged);
		SLTargetLockComponent->OnLockReleased.AddDynamic(this, &ASLCharacterBase::HandleLockReleased);
		SLTargetLockComponent->OnTargetSwitched.AddDynamic(this, &ASLCharacterBase::HandleLockIndicatorChanged);
	}
}

UAbilitySystemComponent* ASLCharacterBase::GetAbilitySystemComponent() const
{
	return SLAbilitySystemComponent;
}

void ASLCharacterBase::SetComboWindowOpen(bool bOpen)
{
	if (RHCombatComponent)
	{
		RHCombatComponent->SetComboWindowOpen(bOpen);
	}
}

void ASLCharacterBase::SetPreInputWindowOpen(bool bOpen)
{
	if (RHCombatComponent)
	{
		RHCombatComponent->SetPreInputWindowOpen(bOpen);
	}
}

bool ASLCharacterBase::HasPendingAction() const
{
	return RHCombatComponent ? RHCombatComponent->HasPendingAction() : false;
}

bool ASLCharacterBase::CanConsumeOnom(int32 Amount) const
{
	return RHOnomComponent ? RHOnomComponent->CanConsumeOnom(Amount) : false;
}

bool ASLCharacterBase::TryConsumeOnom(int32 Amount)
{
	return RHOnomComponent ? RHOnomComponent->TryConsumeOnom(Amount) : false;
}

void ASLCharacterBase::StartExecution(AActor* Enemy)
{
	if (!RHCombatComponent || !RHCombatComponent->WeaponDefinition || !RHCombatComponent->WeaponDefinition->ExecutionMontage)
	{
		return;
	}

	ActiveExecutionMontage = RHCombatComponent->WeaponDefinition->ExecutionMontage.Get();

	RHCombatComponent->CancelAction();

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		DisableInput(PC);
	}

	PlayAnimMontage(ActiveExecutionMontage);
}

void ASLCharacterBase::HandleDeflected(AActor* Enemy)
{
	if (RHCombatComponent)
	{
		// 被弹开也算玩家命中：与攻击命中同语义——HitGainMode=Onom 按主音规则（AttackHitRule）给音形；
		// HitGainMode=Resonance 则攒共鸣（层+1，类型按主音规则，衰减按武器 DA），不再硬编码给 Onom。
		if (RHOnomComponent && RHCombatComponent->WeaponDefinition)
		{
			URHWeaponDefinition* WeaponDef = RHCombatComponent->WeaponDefinition;
			if (WeaponDef->HitGainMode == ERHOnomHitGainMode::Resonance)
			{
				RHOnomComponent->AddResonanceLayer(
					URHOnomComponent::GetPolarityFromValue(WeaponDef->AttackHitRule.Type),
					WeaponDef->ResonanceDecayRate);
			}
			else
			{
				RHOnomComponent->AddOnom(WeaponDef->AttackHitRule, Enemy);
			}
		}
		RHCombatComponent->PlayDefensiveCameraShake(ERHDefensiveShakeType::Parry);
		RHCombatComponent->PlayDefensiveBreakReaction(Enemy);
	}
}

void ASLCharacterBase::HandleLockIndicatorChanged(AActor* Target)
{
	if (ARHEnemyBase* Old = Cast<ARHEnemyBase>(LockIndicatorTarget.Get()))
	{
		Old->SetLockIndicatorVisible(false);
	}
	LockIndicatorTarget = Target;
	if (ARHEnemyBase* New = Cast<ARHEnemyBase>(Target))
	{
		New->SetLockIndicatorVisible(true);
	}
}

void ASLCharacterBase::HandleLockReleased(AActor* Target)
{
	// 锁定解除：只隐藏旧目标，绝不把 Target 当作新锁定目标重新点亮白点。
	if (ARHEnemyBase* Old = Cast<ARHEnemyBase>(LockIndicatorTarget.Get()))
	{
		Old->SetLockIndicatorVisible(false);
	}
	LockIndicatorTarget = nullptr;
}

void ASLCharacterBase::NotifyEnemyDefeated(AActor* Enemy)
{
	if (SLTargetLockComponent && Enemy && SLTargetLockComponent->GetLockedTarget() == Enemy)
	{
		SLTargetLockComponent->BreakLock();
	}
}

void ASLCharacterBase::SetExecutionAvailable(bool bAvailable, AActor* Enemy)
{
	bExecutionAvailable = bAvailable;
	ExecutionEnemy = bAvailable ? Enemy : nullptr;
}

bool ASLCharacterBase::TryStartExecution()
{
	if (!bExecutionAvailable || !ExecutionEnemy)
	{
		return false;
	}
	if (!RHCombatComponent || !RHCombatComponent->WeaponDefinition)
	{
		return false;
	}

	URHWeaponDefinition* WeaponDef = RHCombatComponent->WeaponDefinition;
	UAnimMontage* PlayerMontage = WeaponDef->ExecutionMontage.Get();
	UAnimMontage* EnemyMontage = WeaponDef->ExecutedMontage.Get();
	if (!PlayerMontage || !EnemyMontage)
	{
		return false;
	}

	// 把玩家挪到敌人正前方。
	const FVector EnemyLocation = ExecutionEnemy->GetActorLocation();
	const FVector EnemyForward = ExecutionEnemy->GetActorForwardVector().GetSafeNormal2D();
	const FVector Target = EnemyLocation + EnemyForward * FMath::Max(WeaponDef->ExecutedDistance, 0.f);
	SetActorLocation(Target, false, nullptr, ETeleportType::TeleportPhysics);

	const FVector ToEnemy = (EnemyLocation - GetActorLocation()).GetSafeNormal2D();
	if (!ToEnemy.IsNearlyZero())
	{
		SetActorRotation(ToEnemy.Rotation());
	}
	if (ExecutionEnemy)
	{
		const FVector ToPlayer = (GetActorLocation() - EnemyLocation).GetSafeNormal2D();
		if (!ToPlayer.IsNearlyZero())
		{
			ExecutionEnemy->SetActorRotation(ToPlayer.Rotation());
		}
	}

	RHCombatComponent->CancelAction();
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		DisableInput(PC);
	}
	ActiveExecutionMontage = PlayerMontage;
	PlayAnimMontage(PlayerMontage);

	// 通知敌人播放处决蒙太奇并开始结算。
	if (URHEnemyAIComponent* EnemyAI = ExecutionEnemy->FindComponentByClass<URHEnemyAIComponent>())
	{
		EnemyAI->BeginExecution();
	}
	if (URHEnemyCombatComponent* EnemyCombat = ExecutionEnemy->FindComponentByClass<URHEnemyCombatComponent>())
	{
		EnemyCombat->PlayExecutionMontage(EnemyMontage);
	}

	bExecutionAvailable = false;
	ExecutionEnemy = nullptr;
	return true;
}

void ASLCharacterBase::HandleExecutionMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage != ActiveExecutionMontage)
	{
		return;
	}

	ActiveExecutionMontage = nullptr;

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		EnableInput(PC);
	}
}

bool ASLCharacterBase::IsMoveInputOppositeFacing() const
{
	// 战斗动作中（闪避/攻击/战技等）不触发快速转身，避免向后闪避误触发。
	if (RHCombatComponent && !RHCombatComponent->IsIdle())
	{
		return false;
	}

	// 急变确认期（手柄横扫换向/快速反打）：实际速度方向在 VelocityFlipWindow 内急转 ≥
	// VelocityFlipAngle，本身就是"玩家明确反向"的强信号，直接判定反向。常规的
	// "输入 vs 朝向"夹角会被 bOrientRotationToMovement 的朝向追手实时吃掉，
	// 手柄横扫时永远达不到阈值，只有速度急变信号是可靠的。
	if (IsMoveInputFlipActive())
	{
		return true;
	}

	const FVector2D MoveInput = GetBufferedMoveInputValue();
	if (MoveInput.IsNearlyZero())
	{
		return false;
	}

	const FVector Direction = ConvertMoveInputToWorldDirection(MoveInput);
	if (Direction.IsNearlyZero())
	{
		return false;
	}

	const float DeltaYaw = FMath::FindDeltaAngleDegrees(GetActorRotation().Yaw, Direction.Rotation().Yaw);
	return FMath::Abs(DeltaYaw) >= SLQuickTurnAngle;
}
