#include "KnsTargetLockComponent.h"
#include "Demo/Combat/RHTargetLockInterface.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"

UKnsTargetLockComponent::UKnsTargetLockComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
}

void UKnsTargetLockComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!OwnerCharacter)
	{
		UE_LOG(LogTemp, Warning, TEXT("KnsTargetLockComponent: Owner is not an ACharacter."));
		SetComponentTickEnabled(false);
	}
}

// ========================================================================
//  Public API
// ========================================================================

bool UKnsTargetLockComponent::ToggleLock()
{
	if (IsLocked())
	{
		BreakLock();
		return false;
	}
	return TryLock();
}

bool UKnsTargetLockComponent::TryLock()
{
	const TArray<AActor*> Candidates = FindLockableTargets();
	if (Candidates.IsEmpty())
	{
		return false;
	}

	AActor* Best = PickBestTarget(Candidates);
	if (!Best)
	{
		return false;
	}

	LockedTarget = Best;
	OffScreenTimer = 0.f;

	OnLockAcquired.Broadcast(Best);
	OnLockStateChanged.Broadcast(Best);
	return true;
}

void UKnsTargetLockComponent::BreakLock()
{
	if (!IsLocked())
	{
		return;
	}

	AActor* PrevTarget = LockedTarget.Get();
	if (IRHTargetLockInterface* Lockable = Cast<IRHTargetLockInterface>(PrevTarget))
	{
		Lockable->NotifyTargetLockChanged(false);
	}
	LockedTarget.Reset();
	OffScreenTimer = 0.f;

	OnLockReleased.Broadcast(PrevTarget);
	OnLockStateChanged.Broadcast(nullptr);
}

void UKnsTargetLockComponent::SwitchTarget(const float Direction)
{
	if (!IsLocked())
	{
		return;
	}

	// Cooldown: skip if called too soon after the last switch
	if (SwitchCooldown > 0.f)
	{
		const float Now = GetWorld()->GetTimeSeconds();
		if (Now - LastSwitchTime < SwitchCooldown)
		{
			return;
		}
		LastSwitchTime = Now;
	}

	AActor* Next = FindSwitchTarget(Direction);
	if (!Next || Next == LockedTarget.Get())
	{
		return;
	}

	if (IRHTargetLockInterface* OldLockable = Cast<IRHTargetLockInterface>(LockedTarget.Get()))
	{
		OldLockable->NotifyTargetLockChanged(false);
	}

	LockedTarget = Next;
	OffScreenTimer = 0.f;

	OnTargetSwitched.Broadcast(Next);
}

// ========================================================================
//  Tick — auto-break checks
// ========================================================================

void UKnsTargetLockComponent::TickComponent(const float DeltaTime, const ELevelTick TickType,
                                            FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!IsLocked() || !OwnerCharacter)
	{
		return;
	}

	AActor* Target = LockedTarget.Get();

	// 1. Target invalid / pending kill → break
	if (!IsValid(Target))
	{
		BreakLock();
		return;
	}

	const FVector MyLoc = OwnerCharacter->GetActorLocation();
	const FVector TargetLoc = Target->GetActorLocation();

	// 2. Distance exceeds break threshold
	if (FVector::DistSquared(MyLoc, TargetLoc) > FMath::Square(BreakDistance))
	{
		BreakLock();
		return;
	}

	// 3. Debug line to locked target (independent of LOS check)
	const FVector ViewOrigin = GetViewOrigin();

	if (bShowDebug)
	{
		DrawDebugLine(GetWorld(), ViewOrigin, TargetLoc, FColor::Red, false, -1.f, 0, 1.f);
	}

	// 4. Line-of-sight check
	if (bBreakOnLineOfSightLoss)
	{
		FHitResult Hit;
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(OwnerCharacter);
		Params.AddIgnoredActor(Target);

		const bool bHit = GetWorld()->LineTraceSingleByChannel(
			Hit, ViewOrigin, TargetLoc, ECC_Visibility, Params);

		if (bHit)
		{
			if (bShowDebug)
			{
				DrawDebugPoint(GetWorld(), Hit.ImpactPoint, 10.f, FColor::Yellow, false, -1.f);
			}
			BreakLock();
			return;
		}
	}

	// 5. Off-screen timer
	if (OffScreenBreakDelay > 0.f)
	{
		if (APlayerController* PC = GetOwnerController())
		{
			FVector2D ScreenPos;
			if (ProjectToScreen(PC, TargetLoc, ScreenPos))
			{
				static constexpr float ScreenMargin = 0.05f;
				const bool bOnScreen = ScreenPos.X > -ScreenMargin && ScreenPos.X < 1.f + ScreenMargin
				                    && ScreenPos.Y > -ScreenMargin && ScreenPos.Y < 1.f + ScreenMargin;
				if (bOnScreen)
				{
					OffScreenTimer = 0.f;
				}
				else
				{
					OffScreenTimer += DeltaTime;
				}
			}
			else
			{
				// Behind camera — definitely off-screen, accumulate time.
				OffScreenTimer += DeltaTime;
			}
		}

		if (OffScreenTimer >= OffScreenBreakDelay)
		{
			BreakLock();
			return;
		}
	}
}

// ========================================================================
//  Internal — Find candidates
// ========================================================================

TArray<AActor*> UKnsTargetLockComponent::FindLockableTargets() const
{
	if (!OwnerCharacter)
	{
		return {};
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return {};
	}

	// Search from the character, forward = camera direction
	const FVector SearchOrigin = OwnerCharacter->GetActorLocation();
	const FVector Forward = GetViewDirection();

	// Sphere overlap to collect nearby actors on our trace channel
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(TargetTraceChannel.GetValue()));

	TArray<AActor*> Ignored;
	Ignored.Add(OwnerCharacter);

	TArray<AActor*> OutActors;
	UKismetSystemLibrary::SphereOverlapActors(
		World,
		SearchOrigin,
		LockRange,
		ObjectTypes,
		AActor::StaticClass(),
		Ignored,
		OutActors);

	// Filter: must be within forward cone + pass tag filter + pass Blueprint filter
	OutActors.RemoveAll([&](const AActor* A)
	{
		if (!IsValid(A))
		{
			return true;
		}
		// Interface filter (ignored if RequiredLockInterface is None)
		if (RequiredLockInterface && !A->GetClass()->ImplementsInterface(RequiredLockInterface))
		{
			return true;
		}
		const FVector ToTarget = (A->GetActorLocation() - SearchOrigin).GetSafeNormal();
		const float Dot = FVector::DotProduct(Forward, ToTarget);
		const float MinDot = FMath::Cos(FMath::DegreesToRadians(LockConeHalfAngle));
		if (Dot < MinDot)
		{
			return true;
		}
		// Blueprint-overridable filter (team, alive, tag, etc.)
		if (!IsActorLockable(const_cast<AActor*>(A)))
		{
			return true;
		}
		return false;
	});

	return OutActors;
}

// ========================================================================
//  Internal — Pick best initial target (angular, two-tier)
// ========================================================================

AActor* UKnsTargetLockComponent::PickBestTarget(const TArray<AActor*>& Candidates) const
{
	if (!OwnerCharacter)
	{
		return nullptr;
	}

	const FVector CharLoc = OwnerCharacter->GetActorLocation();
	const float ThresholdRad = FMath::DegreesToRadians(ScreenCenterThreshold);

	struct FScored
	{
		AActor* Actor = nullptr;
		float AngularDist = 0.f;   // radians from camera centre, or BIG_NUMBER if behind
		float WorldDistSq = 0.f;  // squared distance from character
	};

	static constexpr float BehindPenalty = 999.f;

	TArray<FScored> Scored;
	Scored.Reserve(Candidates.Num());

	for (AActor* Candidate : Candidates)
	{
		if (!IsValid(Candidate))
		{
			continue;
		}

		const FVector TargetLoc = Candidate->GetActorLocation();

		float HorizAngle = 0.f, VertAngle = 0.f;
		float AngularDist;
		if (ComputeAngularOffset(TargetLoc, HorizAngle, VertAngle))
		{
			AngularDist = FMath::Sqrt(HorizAngle * HorizAngle + VertAngle * VertAngle);
		}
		else
		{
			AngularDist = BehindPenalty; // behind camera
		}

		const float WorldDistSq = FVector::DistSquared(CharLoc, TargetLoc);
		Scored.Add({Candidate, AngularDist, WorldDistSq});
	}

	if (Scored.IsEmpty())
	{
		return nullptr;
	}

	// ---- Tier 1: candidates within threshold → pick closest to player ----
	FScored* BestWithin = nullptr;
	for (FScored& S : Scored)
	{
		if (S.AngularDist <= ThresholdRad)
		{
			if (!BestWithin || S.WorldDistSq < BestWithin->WorldDistSq)
			{
				BestWithin = &S;
			}
		}
	}

	AActor* Result = nullptr;

	if (BestWithin)
	{
		Result = BestWithin->Actor;
	}
	else
	{
		// ---- Tier 2: all outside threshold → pick closest to camera centre ----
		FScored* BestByAngle = nullptr;
		for (FScored& S : Scored)
		{
			if (!BestByAngle || S.AngularDist < BestByAngle->AngularDist)
			{
				BestByAngle = &S;
			}
		}
		if (BestByAngle)
		{
			Result = BestByAngle->Actor;
		}
	}

	// ---- Debug: draw search cone + highlight best target ----
	if (bShowDebug)
	{
		UWorld* World = GetWorld();
		if (World)
		{
			const FVector DebugViewOrigin = GetViewOrigin();
			const FVector ViewDir = GetViewDirection();
			const FVector CamRight = FVector::CrossProduct(FVector::UpVector, ViewDir).GetSafeNormal();
			const FVector CamUp    = FVector::CrossProduct(ViewDir, CamRight);
			const float HalfAngleRad = FMath::DegreesToRadians(LockConeHalfAngle);
			const int32 Segments = 16;

			TArray<FVector> RingPoints;
			RingPoints.Reserve(Segments);

			for (int32 i = 0; i < Segments; ++i)
			{
				const float Angle = (float)i / Segments * 2.f * PI;
				const FVector ConeDir = ViewDir * FMath::Cos(HalfAngleRad)
					+ (CamRight * FMath::Cos(Angle) + CamUp * FMath::Sin(Angle)) * FMath::Sin(HalfAngleRad);
				const FVector EndPoint = DebugViewOrigin + ConeDir * LockRange;
				RingPoints.Add(EndPoint);

				// Line from origin to ring
				DrawDebugLine(World, DebugViewOrigin, EndPoint, FColor::Green, false, DebugConeDuration, 0, 1.f);
			}

			// Connect ring segments
			for (int32 i = 0; i < Segments; ++i)
			{
				const int32 Next = (i + 1) % Segments;
				DrawDebugLine(World, RingPoints[i], RingPoints[Next], FColor::Green, false, DebugConeDuration, 0, 1.f);
			}

			// Forward centre line
			DrawDebugLine(World, DebugViewOrigin, DebugViewOrigin + ViewDir * LockRange, FColor::Cyan, false, DebugConeDuration, 0, 1.f);

			// Highlight best target
			if (Result)
			{
				DrawDebugSphere(World, Result->GetActorLocation(), 50.f, 12, FColor::Yellow, false, DebugConeDuration);
				DrawDebugLine(World, DebugViewOrigin, Result->GetActorLocation(), FColor::Yellow, false, DebugConeDuration, 0, 2.f);
			}
		}
	}

	return Result;
}

// ========================================================================
//  Internal — Switch target (angular, next-neighbour)
// ========================================================================

AActor* UKnsTargetLockComponent::FindSwitchTarget(const float Direction) const
{
	if (!OwnerCharacter || !IsLocked())
	{
		return nullptr;
	}

	AActor* CurrentTarget = LockedTarget.Get();
	if (!CurrentTarget)
	{
		return nullptr;
	}

	// Get horizontal angle of current target relative to camera
	float CurrentHoriz = 0.f, CurrentVert = 0.f;
	if (!ComputeAngularOffset(CurrentTarget->GetActorLocation(), CurrentHoriz, CurrentVert))
	{
		return nullptr;
	}

	const TArray<AActor*> Candidates = FindLockableTargets();
	if (Candidates.IsEmpty())
	{
		return nullptr;
	}

	struct FSwitchCand
	{
		AActor* Actor = nullptr;
		float HorizAngle = 0.f;
		float VertAngle = 0.f;
	};

	TArray<FSwitchCand> SideCands;

	for (AActor* Candidate : Candidates)
	{
		if (Candidate == CurrentTarget || !IsValid(Candidate))
		{
			continue;
		}

		float H = 0.f, V = 0.f;
		if (!ComputeAngularOffset(Candidate->GetActorLocation(), H, V))
		{
			continue;
		}

		const float Delta = H - CurrentHoriz;

		// Direction > 0 → right (positive delta); Direction < 0 → left (negative delta)
		if (FMath::Sign(Delta) != FMath::Sign(Direction))
		{
			continue;
		}

		SideCands.Add({Candidate, H, V});
	}

	if (SideCands.IsEmpty())
	{
		return nullptr;
	}

	// Sort by absolute horizontal angular distance — nearest first
	SideCands.Sort([CurrentHoriz](const FSwitchCand& A, const FSwitchCand& B)
	{
		return FMath::Abs(A.HorizAngle - CurrentHoriz) < FMath::Abs(B.HorizAngle - CurrentHoriz);
	});

	return SideCands[0].Actor;
}

// ========================================================================
//  Internal — Utility
// ========================================================================

APlayerController* UKnsTargetLockComponent::GetOwnerController() const
{
	if (!OwnerCharacter)
	{
		return nullptr;
	}
	return Cast<APlayerController>(OwnerCharacter->GetController());
}

bool UKnsTargetLockComponent::ProjectToScreen(APlayerController* PC, const FVector& WorldPos,
                                              FVector2D& OutScreenPos) const
{
	if (!PC)
	{
		return false;
	}

	return PC->ProjectWorldLocationToScreen(WorldPos, OutScreenPos, true);
}

bool UKnsTargetLockComponent::ComputeAngularOffset(const FVector& WorldPos,
                                                   float& OutHorizAngle, float& OutVertAngle) const
{
	const FVector CamLoc = GetViewOrigin();
	const FVector CamForward = GetViewDirection();

	// Camera basis vectors — right and up in world space
	const FVector CamRight = FVector::CrossProduct(FVector::UpVector, CamForward).GetSafeNormal();
	const FVector CamUp    = FVector::CrossProduct(CamForward, CamRight);

	const FVector Dir = (WorldPos - CamLoc).GetSafeNormal();

	// Must be in front of the camera plane
	if (FVector::DotProduct(Dir, CamForward) <= 0.f)
	{
		return false;
	}

	// sin(angle) = projection onto the perpendicular axis
	OutHorizAngle = FMath::Asin(FVector::DotProduct(Dir, CamRight));
	OutVertAngle  = FMath::Asin(FVector::DotProduct(Dir, CamUp));

	return true;
}

bool UKnsTargetLockComponent::IsInForwardCone(const FVector& WorldPos) const
{
	const FVector Origin = GetViewOrigin();
	const FVector Forward = GetViewDirection();
	const FVector ToTarget = (WorldPos - Origin).GetSafeNormal();
	const float Dot = FVector::DotProduct(Forward, ToTarget);
	return Dot >= FMath::Cos(FMath::DegreesToRadians(LockConeHalfAngle));
}

FVector UKnsTargetLockComponent::GetViewDirection() const
{
	if (bUseOwnerRotation && OwnerCharacter)
	{
		return OwnerCharacter->GetControlRotation().Vector();
	}

	// Prefer the actual camera from PlayerCameraManager — this gives the true
	// rendered view, not the potentially-lagging control rotation.
	if (const APlayerController* PC = GetOwnerController())
	{
		if (APlayerCameraManager* CamMgr = PC->PlayerCameraManager)
		{
			return CamMgr->GetCameraRotation().Vector();
		}

		return PC->GetControlRotation().Vector();
	}

	return OwnerCharacter ? OwnerCharacter->GetActorForwardVector() : FVector::ForwardVector;
}

FVector UKnsTargetLockComponent::GetViewOrigin() const
{
	if (bUseOwnerRotation && OwnerCharacter)
	{
		FVector OutLocation;
		FRotator OutRotation;
		OwnerCharacter->GetActorEyesViewPoint(OutLocation, OutRotation);
		return OutLocation;
	}

	// Prefer the actual camera position from PlayerCameraManager.
	if (const APlayerController* PC = GetOwnerController())
	{
		if (APlayerCameraManager* CamMgr = PC->PlayerCameraManager)
		{
			return CamMgr->GetCameraLocation();
		}

		FVector OutLocation;
		FRotator OutRotation;
		PC->GetPlayerViewPoint(OutLocation, OutRotation);
		return OutLocation;
	}

	return OwnerCharacter ? OwnerCharacter->GetActorLocation() : FVector::ZeroVector;
}

FVector UKnsTargetLockComponent::GetCameraLocation() const
{
	if (const APlayerController* PC = GetOwnerController())
	{
		if (APlayerCameraManager* CamMgr = PC->PlayerCameraManager)
		{
			return CamMgr->GetCameraLocation();
		}
	}
	// Fallback to view origin if no camera manager available
	return GetViewOrigin();
}

// ========================================================================
//  BlueprintNativeEvent — default filter
// ========================================================================

bool UKnsTargetLockComponent::IsActorLockable_Implementation(AActor* Actor) const
{
	// Default: any actor that passed the collision + cone check is lockable.
	// Override in Blueprint to add your own rules (team, alive, etc.).
	return IsValid(Actor);
}

// ========================================================================
//  Getters
// ========================================================================

bool UKnsTargetLockComponent::GetLockedTargetLocation(FVector& OutLocation) const
{
	AActor* Target = LockedTarget.Get();
	if (!IsValid(Target))
	{
		OutLocation = FVector::ZeroVector;
		return false;
	}

	OutLocation = Target->GetActorLocation();
	return true;
}

float UKnsTargetLockComponent::GetDistanceToTarget() const
{
	AActor* Target = LockedTarget.Get();
	if (!IsValid(Target) || !OwnerCharacter)
	{
		return 0.f;
	}

	return FVector::Dist(OwnerCharacter->GetActorLocation(), Target->GetActorLocation());
}

float UKnsTargetLockComponent::GetCameraDistanceToTarget() const
{
	AActor* Target = LockedTarget.Get();
	if (!IsValid(Target))
	{
		return 0.f;
	}

	return FVector::Dist(GetCameraLocation(), Target->GetActorLocation());
}

float UKnsTargetLockComponent::GetDistanceToTarget2D() const
{
	AActor* Target = LockedTarget.Get();
	if (!IsValid(Target) || !OwnerCharacter)
	{
		return 0.f;
	}

	return FVector::Dist2D(OwnerCharacter->GetActorLocation(), Target->GetActorLocation());
}

FRotator UKnsTargetLockComponent::GetLookAtRotation() const
{
	AActor* Target = LockedTarget.Get();
	if (!IsValid(Target) || !OwnerCharacter)
	{
		return FRotator::ZeroRotator;
	}

	return UKismetMathLibrary::FindLookAtRotation(OwnerCharacter->GetActorLocation(), Target->GetActorLocation());
}

FRotator UKnsTargetLockComponent::GetCameraLookAtRotation() const
{
	AActor* Target = LockedTarget.Get();
	if (!IsValid(Target))
	{
		return FRotator::ZeroRotator;
	}

	return UKismetMathLibrary::FindLookAtRotation(GetCameraLocation(), Target->GetActorLocation());
}

FVector UKnsTargetLockComponent::GetMidPoint(const float Ratio) const
{
	const FVector OwnerLoc = OwnerCharacter ? OwnerCharacter->GetActorLocation() : FVector::ZeroVector;
	AActor* Target = LockedTarget.Get();
	if (!IsValid(Target))
	{
		return OwnerLoc;
	}

	return FMath::Lerp(OwnerLoc, Target->GetActorLocation(), FMath::Clamp(Ratio, 0.f, 1.f));
}

bool UKnsTargetLockComponent::IsTargetWithinDistance(const float Distance) const
{
	AActor* Target = LockedTarget.Get();
	if (!IsValid(Target) || !OwnerCharacter)
	{
		return false;
	}

	return FVector::DistSquared(OwnerCharacter->GetActorLocation(), Target->GetActorLocation()) <= FMath::Square(Distance);
}
