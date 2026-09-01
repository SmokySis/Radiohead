#include "KnsMoveRotationAdjustNotifyState.h"

#include "Animation/AnimSequenceBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedPlayerInput.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "KnsComboComponent.h"

void UKnsMoveRotationAdjustNotifyState::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
	AccumulatedYaw = 0.f;
}

void UKnsMoveRotationAdjustNotifyState::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

	AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	if (!Owner || !MoveInputAction)
	{
		return;
	}

	APawn* Pawn = Cast<APawn>(Owner);
	APlayerController* PlayerController = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	ULocalPlayer* LocalPlayer = PlayerController ? PlayerController->GetLocalPlayer() : nullptr;
	UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer ? ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer) : nullptr;
	UEnhancedPlayerInput* PlayerInput = InputSubsystem ? InputSubsystem->GetPlayerInput() : nullptr;

	const FInputActionValue ActionValue = PlayerInput ? PlayerInput->GetActionValue(MoveInputAction) : FInputActionValue();
	const FVector2D MoveInput = ActionValue.Get<FVector2D>();
	const float InputStrength = MoveInput.Size();

	if (InputStrength < DeadZone)
	{
		return;
	}

	const FVector WorldMove = UKnsComboComponent::GetWorldMoveInputDirection(Owner, MoveInput, DeadZone);
	if (WorldMove.IsNearlyZero())
	{
		return;
	}

	if (bDrawDebug && MeshComp)
	{
		if (UWorld* World = MeshComp->GetWorld())
		{
			const FVector Start = Owner->GetActorLocation();
			DrawDebugDirectionalArrow(World, Start, Start + WorldMove * 100.f, 20.f, FColor::Yellow, false, 0.05f);
		}
	}

	const float TargetYaw = WorldMove.Rotation().Yaw;
	const FRotator CurrentRotation = Owner->GetActorRotation();
	const float YawDelta = FMath::FindDeltaAngleDegrees(CurrentRotation.Yaw, TargetYaw);

	float MaxStep = RotationSpeed * FrameDeltaTime * RotationCoefficient * FMath::Clamp(InputStrength, 0.f, 1.f);

	if (MaxTotalYaw > 0.f)
	{
		const float RemainingYaw = FMath::Max(0.f, MaxTotalYaw - FMath::Abs(AccumulatedYaw));
		MaxStep = FMath::Min(MaxStep, RemainingYaw);
		if (MaxStep <= 0.f)
		{
			return;
		}
	}

	const float AppliedDelta = FMath::Clamp(YawDelta, -MaxStep, MaxStep);
	if (FMath::IsNearlyZero(AppliedDelta))
	{
		return;
	}

	AccumulatedYaw += FMath::Abs(AppliedDelta);

	FRotator NewRotation = CurrentRotation;
	NewRotation.Yaw += AppliedDelta;
	Owner->SetActorRotation(NewRotation);
}

void UKnsMoveRotationAdjustNotifyState::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);
	AccumulatedYaw = 0.f;
}

FString UKnsMoveRotationAdjustNotifyState::GetNotifyName_Implementation() const
{
	return TEXT("Kns Move Rotation Adjust");
}
