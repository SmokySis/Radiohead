#include "BaseCharacter.h"

#include "AbilitySystemComponent.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedPlayerInput.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "Math/RotationMatrix.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameplayTagContainer.h"

namespace
{
	// Turn-in-place conventions (hardcoded by agreement).
	const FName TurnRightSection(TEXT("TurnR"));       // right 90°
	const FName TurnLeftSection(TEXT("TurnL"));        // left 90°
	const FName TurnBackSection(TEXT("TurnB"));        // back 180°
	constexpr float TurnForwardAngle = 45.f;           // |yaw delta| <= this → forward, no turn
	constexpr float TurnBackAngle = 135.f;             // |yaw delta| >= this → back (TurnB)
	constexpr float QuickTurnAngle = 135.f;            // |yaw delta| >= this → opposite (quick turn)
}

ABaseCharacter::ABaseCharacter()
{
}

UAbilitySystemComponent* ABaseCharacter::GetAbilitySystemComponent() const
{
	return nullptr;
}

FVector2D ABaseCharacter::GetMoveInputValue() const
{
	if (!MoveInputAction)
	{
		return FVector2D::ZeroVector;
	}

	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	ULocalPlayer* LocalPlayer = PlayerController ? PlayerController->GetLocalPlayer() : nullptr;
	UEnhancedInputLocalPlayerSubsystem* InputSubsystem =
		LocalPlayer ? ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer) : nullptr;
	UEnhancedPlayerInput* PlayerInput = InputSubsystem ? InputSubsystem->GetPlayerInput() : nullptr;

	if (!PlayerInput)
	{
		return FVector2D::ZeroVector;
	}

	const FInputActionValue ActionValue = PlayerInput->GetActionValue(MoveInputAction);
	return ActionValue.Get<FVector2D>();
}

FVector2D ABaseCharacter::GetBufferedMoveInputValue() const
{
	// 急变检测已迁到速度域（见 UpdateVelocityFlipDetection / IsMoveInputFlipActive），
	// 本函数回归纯实时输入：移动、引擎朝向追手等一切实时行为读到的永远是真实输入，
	// 不被任何缓冲/锁存污染。急变信号只通过 IsMoveInputFlipActive 暴露给 pivot/转身判定。
	return GetMoveInputValue();
}

bool ABaseCharacter::IsMoveInputFlipActive() const
{
	// 先推进速度采样（每帧最多一次），再查确认窗。
	UpdateVelocityFlipDetection();

	const UWorld* World = GetWorld();
	const float Now = World ? World->GetTimeSeconds() : -1.f;
	if (bMoveInputFlipped && Now >= 0.f && Now <= MoveInputFlipExpireTime)
	{
		return true;
	}
	// 窗口过期即清除，下次急转重新触发。
	bMoveInputFlipped = false;
	return false;
}

void ABaseCharacter::UpdateVelocityFlipDetection() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const float Now = World->GetTimeSeconds();
	const float Delta = World->GetDeltaSeconds();

	// 同帧去重：每帧最多推进一次采样，pivot/quick turn/BP 直查等多路调用不会重复累积。
	if (LastVelocitySampleTime >= 0.f && (Now - LastVelocitySampleTime) < Delta * 0.5f)
	{
		return;
	}
	LastVelocitySampleTime = Now;

	const FVector Velocity = GetVelocity();
	const float Speed = Velocity.Size2D();

	// 速度死区：静止/起步时速度方向无意义且噪声大，直接清空历史不检测。
	if (Speed < VelocityFlipMinSpeed)
	{
		VelocityFlipHistory.Reset();
		VelocityFlipHistoryTime.Reset();
		return;
	}

	const FVector2D CurDir = FVector2D(Velocity.X, Velocity.Y).GetSafeNormal();

	// 窗口滑动：只保留 VelocityFlipWindow 内的样本，丢弃窗口外的旧样本。
	VelocityFlipHistory.Add(CurDir);
	VelocityFlipHistoryTime.Add(Now);
	int32 FirstValidIndex = 0;
	while (FirstValidIndex < VelocityFlipHistory.Num() &&
		   Now - VelocityFlipHistoryTime[FirstValidIndex] > VelocityFlipWindow)
	{
		++FirstValidIndex;
	}
	if (FirstValidIndex > 0)
	{
		VelocityFlipHistory.RemoveAt(0, FirstValidIndex, EAllowShrinking::No);
		VelocityFlipHistoryTime.RemoveAt(0, FirstValidIndex, EAllowShrinking::No);
	}

	// 累积窗口内相邻样本夹角（度）：键盘秒切 ≈ 180°，手柄横扫 ≈ 120°+，
	// 普通转弯/绕圈远低于阈值，慢转永远不会累积到。
	float AccumAngle = 0.f;
	for (int32 i = 1; i < VelocityFlipHistory.Num(); ++i)
	{
		const float Dot = FMath::Clamp(VelocityFlipHistory[i].Dot(VelocityFlipHistory[i - 1]), -1.f, 1.f);
		AccumAngle += FMath::RadiansToDegrees(FMath::Acos(Dot));
	}

	if (AccumAngle >= VelocityFlipAngle)
	{
		bMoveInputFlipped = true;
		MoveInputFlipExpireTime = Now + VelocityFlipLockDuration;
		// 触发后清空历史，避免同一段急转在确认窗内重复触发。
		VelocityFlipHistory.Reset();
		VelocityFlipHistoryTime.Reset();
	}
}

FVector ABaseCharacter::ConvertMoveInputToWorldDirection(const FVector2D& MoveInput, float DeadZone) const
{
	if (MoveInput.IsNearlyZero() || (DeadZone > 0.f && MoveInput.SizeSquared() < FMath::Square(DeadZone)))
	{
		return FVector::ZeroVector;
	}

	const AController* OwnerController = GetController();
	if (!OwnerController)
	{
		return FVector::ZeroVector;
	}

	const FRotator ControlRotation = OwnerController->GetControlRotation();
	const FVector ControlForward = ControlRotation.Vector();
	const FVector ControlRight = FRotationMatrix(ControlRotation).GetUnitAxis(EAxis::Y);
	const FVector WorldMove = (ControlForward * MoveInput.Y) + (ControlRight * MoveInput.X);

	return WorldMove.GetSafeNormal2D();
}

FVector ABaseCharacter::GetMoveIntentDirection(float DeadZone) const
{
	return ConvertMoveInputToWorldDirection(GetMoveInputValue(), DeadZone);
}

void ABaseCharacter::UpdateTurnTowardsMoveIntent(float InterpSpeed, bool bUseCorrection)
{
	const FVector Direction = GetMoveIntentDirection();
	const FRotator CurrentRotation = GetActorRotation();

	if (!Direction.IsNearlyZero())
	{
		// Input active: commit this direction as the turn target so the turn
		// keeps going even after the input is released.
		TurnTargetRotation = Direction.Rotation();
		TurnTargetRotation.Pitch = CurrentRotation.Pitch;
		TurnTargetRotation.Roll = CurrentRotation.Roll;
		bTurnInProgress = true;
	}

	if (!bTurnInProgress)
	{
		return;
	}

	const float YawDelta = FMath::Abs(FMath::FindDeltaAngleDegrees(CurrentRotation.Yaw, TurnTargetRotation.Yaw));

	if (YawDelta <= TurnCompleteThreshold)
	{
		SetActorRotation(TurnTargetRotation);
		bTurnInProgress = false;
		return;
	}

	float EffectiveSpeed = InterpSpeed;
	if (bUseCorrection && RotationCorrectionAngle > 0.f)
	{
		EffectiveSpeed = InterpSpeed * (YawDelta / RotationCorrectionAngle);
	}

	const UWorld* World = GetWorld();
	const float DeltaTime = World ? World->GetDeltaSeconds() : 0.f;
	SetActorRotation(FMath::RInterpConstantTo(CurrentRotation, TurnTargetRotation, DeltaTime, EffectiveSpeed));
}

bool ABaseCharacter::TryPlayTurnInPlaceMontage(UAnimMontage* TurnMontage)
{
	if (!TurnMontage)
	{
		return false;
	}

	// 急变确认期（手柄横扫换向/快速反打）：换向奔跑中不播原地转身蒙太奇，
	// 由引擎朝向追手（bOrientRotationToMovement）平滑掉头，避免把角色钉在原地。
	if (IsMoveInputFlipActive())
	{
		return false;
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
	const float AbsDelta = FMath::Abs(DeltaYaw);

	FName Section;
	if (AbsDelta <= TurnForwardAngle)
	{
		return false; // forward, no turn
	}
	if (AbsDelta >= TurnBackAngle)
	{
		Section = TurnBackSection;
	}
	else if (DeltaYaw > 0.f)
	{
		Section = TurnRightSection;
	}
	else
	{
		Section = TurnLeftSection;
	}

	// 已在播放则不重启：反复 PlayAnimMontage 会一直从头重播，角色会被钉在原地。
	USkeletalMeshComponent* OwnerMesh = GetMesh();
	UAnimInstance* AnimInstance = OwnerMesh ? OwnerMesh->GetAnimInstance() : nullptr;
	if (AnimInstance && AnimInstance->Montage_IsPlaying(TurnMontage))
	{
		return false;
	}

	const float Duration = PlayAnimMontage(TurnMontage, 1.f, Section);
	return Duration > 0.f;
}

bool ABaseCharacter::TryPlayQuickTurnMontage(UAnimMontage* QuickTurnMontage)
{
	if (!QuickTurnMontage)
	{
		return false;
	}

	if (GetVelocity().Size2D() <= QuickTurnMinSpeed)
	{
		return false;
	}

	// 战斗动作中（攻击/战技/闪避/防御等）不触发快速转身：按 Busy tag 门控，避免向后闪避误触发。
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		const FGameplayTag BusyTag = FGameplayTag::RequestGameplayTag(TEXT("State.Player.Busy"), false);
		const FGameplayTag InvincibleTag = FGameplayTag::RequestGameplayTag(TEXT("Status.Invincible"), false);
		if ((BusyTag.IsValid() && ASC->HasMatchingGameplayTag(BusyTag)) ||
			(InvincibleTag.IsValid() && ASC->HasMatchingGameplayTag(InvincibleTag)))
		{
			return false;
		}
	}

	USkeletalMeshComponent* OwnerMesh = GetMesh();
	UAnimInstance* AnimInstance = OwnerMesh ? OwnerMesh->GetAnimInstance() : nullptr;
	if (AnimInstance && AnimInstance->Montage_IsPlaying(QuickTurnMontage))
	{
		// Already quick-turning, don't restart it.
		return false;
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
	if (FMath::Abs(DeltaYaw) < QuickTurnAngle)
	{
		// Not opposite to facing, no quick turn.
		return false;
	}

	const float Duration = PlayAnimMontage(QuickTurnMontage);
	return Duration > 0.f;
}

