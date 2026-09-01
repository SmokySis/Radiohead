#include "KnsRotateToMoveInputNotify.h"

#include "Animation/AnimSequenceBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedPlayerInput.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "Demo/Combat/KnsCombatComponent.h"
#include "KnsComboComponent.h"

void UKnsRotateToMoveInputNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

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
	const FVector WorldMove = UKnsComboComponent::GetWorldMoveInputDirection(Owner, MoveInput, DeadZone);

	// 没有有效移动输入时保持当前朝向
	if (WorldMove.IsNearlyZero())
	{
		return;
	}

	if (bDrawDebug && MeshComp)
	{
		if (UWorld* World = MeshComp->GetWorld())
		{
			const FVector Start = Owner->GetActorLocation();
			DrawDebugDirectionalArrow(World, Start, Start + WorldMove * 100.f, 20.f, FColor::Yellow, false, DebugDrawDuration);
			DrawDebugDirectionalArrow(World, Start, Start + Owner->GetActorForwardVector() * 100.f, 20.f, FColor::Red, false, DebugDrawDuration);
		}
	}

	FRotator TargetRotation = WorldMove.Rotation();
	const FRotator CurrentRotation = Owner->GetActorRotation();
	TargetRotation.Pitch = CurrentRotation.Pitch;
	TargetRotation.Roll = CurrentRotation.Roll;
	TargetRotation.Yaw += RotationOffsetYaw;

	if (UKnsCombatComponent* CombatComponent = Owner->FindComponentByClass<UKnsCombatComponent>())
	{
		CombatComponent->StartRotateToMoveInput(TargetRotation, RotationInterpSpeed);
	}
	else
	{
		Owner->SetActorRotation(TargetRotation);
	}
}

FString UKnsRotateToMoveInputNotify::GetNotifyName_Implementation() const
{
	return TEXT("Kns Rotate To Move Input");
}
