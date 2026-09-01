#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "BaseCharacter.generated.h"

class UInputAction;
class UAnimMontage;

UCLASS()
class DEMO_API ABaseCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ABaseCharacter();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	// -------------------------------------------------------------------------
	// Camera-relative movement intent
	// -------------------------------------------------------------------------

	/** The IA_Move input action used to read the current movement intent. Assign this in Blueprint. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character|Movement")
	TObjectPtr<UInputAction> MoveInputAction;

	/** Reads the raw 2D (camera-space) move input from the local player's enhanced input subsystem. */
	UFUNCTION(BlueprintPure, Category = "Character|Movement")
	FVector2D GetMoveInputValue() const;

	/** Converts a camera-space 2D move input into a normalized horizontal world direction relative to the control/camera rotation. */
	UFUNCTION(BlueprintPure, Category = "Character|Movement")
	FVector ConvertMoveInputToWorldDirection(const FVector2D& MoveInput, float DeadZone = 0.15f) const;

	/**
	 * Reads IA_Move and returns the camera-relative world movement intent direction.
	 * Returns a zero vector when there is no effective movement input.
	 * Use this to rotate the character toward the movement direction instead of the
	 * native "orient rotation to movement".
	 */
	UFUNCTION(BlueprintPure, Category = "Character|Movement")
	FVector GetMoveIntentDirection(float DeadZone = 0.15f) const;

	/**
	 * Reference angle (degrees) used to normalize turn speed.
	 * A turn of exactly this angle rotates at the base InterpSpeed; larger turns
	 * rotate proportionally faster so a 180° turn completes as quickly as a 90° one.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character|Movement", meta = (ClampMin = "1.0"))
	float RotationCorrectionAngle = 90.f;

	/** Remaining yaw (degrees) below which a turn is considered complete and snaps to the target. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character|Movement", meta = (ClampMin = "0.05"))
	float TurnCompleteThreshold = 1.f;

	/** Minimum current speed (units/s) required to trigger a quick turn. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character|Movement", meta = (ClampMin = "0.0"))
	float QuickTurnMinSpeed = 750.f;

	/**
	 * Turn/pivot "flip" detection based on the *actual velocity direction* (not input).
	 * Gamepad stick sweeps change direction continuously through the neutral zone and the
	 * character's facing (orient-to-movement) chases the stick, so the "input vs facing"
	 * angle never reliably reaches the turn threshold. Instead, the velocity direction
	 * itself is watched: when the accumulated direction change over the last
	 * VelocityFlipWindow reaches VelocityFlipAngle — a deliberate sweep / quick re-press —
	 * a flip-confirm window of VelocityFlipLockDuration opens (see IsMoveInputFlipActive).
	 * During that window pivot detection reads a firm "opposite" without relying on the
	 * chasing facing angle. Raw movement input is always returned untouched.
	 */

	/** Minimum current speed (units/s) required for velocity-flip detection (dead zone: idle/start-up noise). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character|Movement", meta = (ClampMin = "0.0"))
	float VelocityFlipMinSpeed = 200.f;

	/** Accumulated velocity-direction change (degrees) within the window that counts as a deliberate flip (keyboard re-press ≈ 180°, gamepad sweep ≈ 120°+, normal turns stay far below). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character|Movement", meta = (ClampMin = "0.0"))
	float VelocityFlipAngle = 120.f;

	/** Detection window (seconds): the direction change must accumulate within this time (gamepad sweep is ~0.1-0.15s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character|Movement", meta = (ClampMin = "0.0"))
	float VelocityFlipWindow = 0.15f;

	/** How long (seconds) the flip-confirm window stays open after a flip is detected (pivot reads it during this time). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character|Movement", meta = (ClampMin = "0.0"))
	float VelocityFlipLockDuration = 0.1f;

	/**
	 * Drives turning toward the movement intent direction. Call every frame (e.g. in Tick).
	 * When movement input is present its direction is committed as the turn target, and the
	 * character keeps rotating until it faces that target even if the input is released mid-turn.
	 * When bUseCorrection is true the speed is scaled by (remaining angle / RotationCorrectionAngle)
	 * so a 180° turn completes as quickly as a 90° one.
	 */
	UFUNCTION(BlueprintCallable, Category = "Character|Movement")
	void UpdateTurnTowardsMoveIntent(float InterpSpeed = 360.f, bool bUseCorrection = true);

	/**
	 * Plays a turn-in-place montage section based on the current IA_Move direction.
	 * Section names are hardcoded by convention: "TurnR" (right 90°), "TurnL" (left 90°), "TurnB" (back 180°).
	 * This is a pure player: press-time / hold detection and stopping are handled by the caller (Blueprint).
	 * Returns true if a turn was started, false otherwise (no input / forward).
	 */
	UFUNCTION(BlueprintCallable, Category = "Character|Movement")
	bool TryPlayTurnInPlaceMontage(UAnimMontage* TurnMontage);

	/**
	 * Plays a quick-turn montage when the current IA_Move input points roughly opposite
	 * to the character's facing direction and the character's current speed is above
	 * QuickTurnMinSpeed. Only the montage reference is required; the speed and
	 * opposite-direction checks happen inside. Connect this to IA_Move's Triggered event.
	 * Returns true if the montage was started, false otherwise (invalid montage / no
	 * opposite input / too slow).
	 */
	UFUNCTION(BlueprintCallable, Category = "Character|Movement")
	bool TryPlayQuickTurnMontage(UAnimMontage* QuickTurnMontage);

	/**
	 * Returns the raw camera-space move input (dead-zone applied) for turn/pivot detection.
	 * Kept as a stable entry point for Blueprint: it no longer buffers or detects flips —
	 * flip detection now lives in the velocity domain and is reported via
	 * IsMoveInputFlipActive. Movement/facing reads are always the true real-time input.
	 */
	UFUNCTION(BlueprintPure, Category = "Character|Movement")
	FVector2D GetBufferedMoveInputValue() const;

	/**
	 * True while a flip-confirm window is open: the actual velocity direction changed by at
	 * least VelocityFlipAngle within VelocityFlipWindow (deliberate sweep / quick re-press,
	 * e.g. a gamepad stick sweep that passes through the neutral zone). Pivot/turn detection
	 * treats this window as a firm "opposite direction" signal, which is reliable even
	 * though the "input vs facing" angle is eaten by orient-to-movement chasing.
	 * Consuming this query clears the flag once the window expires.
	 */
	UFUNCTION(BlueprintPure, Category = "Character|Movement")
	bool IsMoveInputFlipActive() const;

private:
	/** Whether a committed turn is still in progress (continues after input is released). */
	bool bTurnInProgress = false;

	/** The committed target rotation to turn toward. */
	FRotator TurnTargetRotation = FRotator::ZeroRotator;

	/**
	 * Advances the velocity-flip sampler (lazy, once per frame). Watches the actual velocity
	 * direction: when the accumulated direction change over the last VelocityFlipWindow
	 * reaches VelocityFlipAngle (gamepad sweep / keyboard re-press), a flip-confirm window
	 * opens (see IsMoveInputFlipActive). Called from IsMoveInputFlipActive so pivot reads
	 * always see a fresh sample, and the caller site (move follows Tick) guarantees velocity
	 * is current.
	 */
	void UpdateVelocityFlipDetection() const;

	// ---- Velocity-flip detection state (actual movement, not input) ----

	/** Sliding window of recent velocity directions (2D, normalized), oldest first. */
	mutable TArray<FVector2D> VelocityFlipHistory;
	/** Sample world times matching VelocityFlipHistory. */
	mutable TArray<float> VelocityFlipHistoryTime;
	/** World time of the last sampler advance (once-per-frame guard). */
	mutable float LastVelocitySampleTime = -1.f;

	/** True while a flip-confirm window is open (see IsMoveInputFlipActive). */
	mutable bool bMoveInputFlipped = false;
	/** World time at which the flip-confirm window expires. */
	mutable float MoveInputFlipExpireTime = -1.f;
};
