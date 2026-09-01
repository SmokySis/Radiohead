#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "KnsTargetLockComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLockStateChangedDel, AActor*, LockedTarget);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTargetSwitchedDel, AActor*, NewTarget);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLockAcquiredDel, AActor*, AcquiredTarget);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLockReleasedDel, AActor*, PreviousTarget);

/**
 * Souls-like target lock component.
 * Attach to a character (player or AI). Provides lock-on, lock-off, and target switching.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class DEMO_API UKnsTargetLockComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UKnsTargetLockComponent();

	// ----- Lock / Unlock -----

	/** Toggle lock: if locked, break; if unlocked, find and lock best target. Returns true if locked after the call. */
	UFUNCTION(BlueprintCallable, Category = "Lock")
	bool ToggleLock();

	/** Lock onto the best available target. Returns true if a target was locked. */
	UFUNCTION(BlueprintCallable, Category = "Lock")
	bool TryLock();

	/** Break current lock. */
	UFUNCTION(BlueprintCallable, Category = "Lock")
	void BreakLock();

	// ----- Target switching -----

	/** Switch to the next target to the left (negative) or right (positive). */
	UFUNCTION(BlueprintCallable, Category = "Lock")
	void SwitchTarget(float Direction);

	// ----- Getters -----

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lock")
	AActor* GetLockedTarget() const { return LockedTarget.Get(); }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lock")
	bool IsLocked() const { return LockedTarget.Get() != nullptr; }

	/** Get the world location of the currently locked target. Returns false if no target is locked. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lock")
	bool GetLockedTargetLocation(FVector& OutLocation) const;

	/** 3D distance from the owning character to the locked target. Returns 0 if no target. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lock")
	float GetDistanceToTarget() const;

	/** 3D distance from the active camera to the locked target. Returns 0 if no target. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lock")
	float GetCameraDistanceToTarget() const;

	/** Horizontal distance (XY only) from the owning character to the locked target. Returns 0 if no target. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lock")
	float GetDistanceToTarget2D() const;

	/** Rotation from the owning character looking at the locked target. Returns (0,0,0) if no target. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lock")
	FRotator GetLookAtRotation() const;

	/** Rotation from the active camera looking at the locked target. Returns (0,0,0) if no target. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lock")
	FRotator GetCameraLookAtRotation() const;

	/** 对峙取景中点：玩家与锁定目标按比例插值（0=玩家位置，0.5=两人中间，1=目标位置）。无锁定时返回玩家位置。 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lock")
	FVector GetMidPoint(float Ratio = 0.5f) const;

	/** Returns true if the locked target is within the given distance from the owning character. Returns false if no target. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lock")
	bool IsTargetWithinDistance(float Distance) const;

	// ----- Blueprint-overridable filter -----

	/**
	 * Override in Blueprint or C++ to decide whether an actor can be locked onto.
	 * Default: returns true (any actor passing the collision trace is lockable).
	 * Use this to filter by team, alive state, actor tag, etc.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Lock|Filter")
	bool IsActorLockable(AActor* Actor) const;

	// ----- Delegates -----

	/**
	 * Fires when a target has been successfully locked (manual or auto-reacquire).
	 * Bind here to apply your "locked-on" character state
	 * (e.g. disable OrientRotationToMovement, enable controller-rotation yaw).
	 */
	UPROPERTY(BlueprintAssignable, Category = "Lock|Events")
	FOnLockAcquiredDel OnLockAcquired;

	/**
	 * Fires when the current lock is released for ANY reason —
	 * manual BreakLock, auto-break (distance / LOS / off-screen), or target death.
	 * The previous target is passed so you can still reference it if needed.
	 * Bind here to restore your original character movement settings.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Lock|Events")
	FOnLockReleasedDel OnLockReleased;

	/**
	 * Fires every time the lock state changes.
	 *   - Lock acquired: passes the new target
	 *   - Lock released: passes nullptr
	 * Useful for UI (show/hide lock-on marker).
	 */
	UPROPERTY(BlueprintAssignable, Category = "Lock|Events")
	FOnLockStateChangedDel OnLockStateChanged;

	/**
	 * Fires when the target is switched while already locked (left/right cycling).
	 * Passes the new target.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Lock|Events")
	FOnTargetSwitchedDel OnTargetSwitched;

	// ----- Config exposed to editor / BP -----

	/** Max distance to search for lockable targets. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lock|Search")
	float LockRange = 1500.f;

	/** Half-angle (degrees) of the forward cone used for finding the initial lock target. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lock|Search")
	float LockConeHalfAngle = 60.f;

	/**
	 * Angular threshold (degrees) from camera centre for two-tier target selection.
	 * - If any candidate's angular offset is within this threshold → pick the
	 *   CLOSEST (world-distance) among those inside.
	 * - Otherwise → pick the one CLOSEST TO CAMERA CENTRE.
	 * A sensible range is 5–15 degrees (inner portion of the screen).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lock|Search", meta = (ClampMin = "0.0", ClampMax = "45.0"))
	float ScreenCenterThreshold = 8.f;

	/** Max distance before auto-breaking the lock (a bit larger than LockRange for hysteresis). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lock|Break")
	float BreakDistance = 2000.f;

	/** Break lock if the target leaves the screen for longer than this (seconds). 0 = no screen check. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lock|Break")
	float OffScreenBreakDelay = 2.f;

	/** If true, break lock when the owner and target lose line of sight. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lock|Break")
	bool bBreakOnLineOfSightLoss = true;

	/** Collision channel used for sphere overlap to find targets. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lock|Search")
	TEnumAsByte<ECollisionChannel> TargetTraceChannel = ECC_Pawn;

	/** If set, only actors implementing this interface are lockable. Leave empty (None) to skip interface filtering. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lock|Search")
	TSubclassOf<UInterface> RequiredLockInterface;

	/** If true, use the owner's control rotation instead of the camera manager's view. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lock|Search")
	bool bUseOwnerRotation = false;

	/** Minimum interval (seconds) between consecutive target switches. 0 = no cooldown (immediate). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lock|Switch")
	float SwitchCooldown = 0.15f;

	/** If true, draw debug lines for search cone + LOS trace. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lock|Debug")
	bool bShowDebug = false;

	/** Duration (seconds) to persist the debug cone drawn during target search. 0 = single frame. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lock|Debug")
	float DebugConeDuration = 0.2f;

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	/** Current locked target (weak ptr — doesn't keep actor alive). */
	UPROPERTY(VisibleAnywhere, Category = "Lock|State")
	TWeakObjectPtr<AActor> LockedTarget;

	/** Timer tracking how long the locked target has been off-screen. */
	float OffScreenTimer = 0.f;

	/** World time when the last target switch occurred (for cooldown). */
	float LastSwitchTime = -1.f;

	/** The character that owns this component (cached). */
	UPROPERTY()
	TObjectPtr<ACharacter> OwnerCharacter;

	// ----- Internal helpers -----

	/** Overlap sphere sweep to collect candidate actors. */
	TArray<AActor*> FindLockableTargets() const;

	/** From a list of candidates, pick the best target using angular distance from camera centre. */
	AActor* PickBestTarget(const TArray<AActor*>& Candidates) const;

	/** Among current lockable targets, find the next one in the given direction. */
	AActor* FindSwitchTarget(float Direction) const;

	/** Return the player controller (through the owner). */
	APlayerController* GetOwnerController() const;

	/** Project world location to viewport-normalised screen position. */
	bool ProjectToScreen(APlayerController* PC, const FVector& WorldPos, FVector2D& OutScreenPos) const;

	/**
	 * Compute horizontal and vertical angular offset (radians) from camera forward.
	 * Returns false if the point is behind the camera plane.
	 */
	bool ComputeAngularOffset(const FVector& WorldPos, float& OutHorizAngle, float& OutVertAngle) const;

	/** Check whether WorldPos is within the forward cone of the view. */
	bool IsInForwardCone(const FVector& WorldPos) const;

	/** Get the view direction (camera forward, or character control rotation if bUseOwnerRotation). */
	FVector GetViewDirection() const;

	/** Get the view origin (camera position, or character eyes if bUseOwnerRotation). */
	FVector GetViewOrigin() const;

	/** Get the active camera location directly from PlayerCameraManager (ignores bUseOwnerRotation). */
	FVector GetCameraLocation() const;
};
