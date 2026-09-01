// Copyright Epic Games, Inc. All Rights Reserved.

#include "LagCameraNode.h"

#include "Core/CameraNodeEvaluator.h"
#include "Core/CameraParameterReader.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LagCameraNode)

namespace UE::Cameras
{

class FLagCameraNodeEvaluator : public FCameraNodeEvaluator
{
	UE_DECLARE_CAMERA_NODE_EVALUATOR(DEMO_API, FLagCameraNodeEvaluator)

protected:

	// FCameraNodeEvaluator interface.
	virtual void OnInitialize(const FCameraNodeEvaluatorInitializeParams& Params, FCameraNodeEvaluationResult& OutResult) override;
	virtual void OnRun(const FCameraNodeEvaluationParams& Params, FCameraNodeEvaluationResult& OutResult) override;
	virtual void OnSerialize(const FCameraNodeEvaluatorSerializeParams& Params, FArchive& Ar) override;

private:

	/** Compute forward/right/up axes and the raw rotation from the configured space. */
	void GetAxes(
		const FCameraNodeEvaluationResult& Result,
		FVector3d& OutForward,
		FVector3d& OutRight,
		FVector3d& OutUp) const;

	TCameraParameterReader<float> ForwardLagSpeedReader;
	TCameraParameterReader<float> LateralLagSpeedReader;
	TCameraParameterReader<float> VerticalLagSpeedReader;
	TCameraParameterReader<float> MaxSnapDistanceReader;

	/**
	 * The lag stored in reference-local space.
	 * X = forward lag, Y = lateral lag, Z = vertical lag.
	 * When the reference rotation changes, these components naturally
	 * rotate with it — no per-axis state corruption.
	 */
	FVector3d LocalLag = FVector3d::ZeroVector;

	/** The target position from the previous frame, for computing delta. */
	FVector3d PreviousTargetPosition = FVector3d::ZeroVector;

	/** Whether we have been initialized. */
	bool bIsInitialized = false;
};

UE_DEFINE_CAMERA_NODE_EVALUATOR(FLagCameraNodeEvaluator)

void FLagCameraNodeEvaluator::OnInitialize(
		const FCameraNodeEvaluatorInitializeParams& Params,
		FCameraNodeEvaluationResult& OutResult)
{
	SetNodeEvaluatorFlags(ECameraNodeEvaluatorFlags::NeedsSerialize);

	const ULagCameraNode* LagNode = GetCameraNodeAs<ULagCameraNode>();
	ForwardLagSpeedReader.Initialize(LagNode->ForwardLagSpeed);
	LateralLagSpeedReader.Initialize(LagNode->LateralLagSpeed);
	VerticalLagSpeedReader.Initialize(LagNode->VerticalLagSpeed);
	MaxSnapDistanceReader.Initialize(LagNode->MaxSnapDistance);
}

void FLagCameraNodeEvaluator::GetAxes(
		const FCameraNodeEvaluationResult& Result,
		FVector3d& OutForward,
		FVector3d& OutRight,
		FVector3d& OutUp) const
{
	const ULagCameraNode* LagNode = GetCameraNodeAs<ULagCameraNode>();

	FRotator3d AxesRotation = FRotator3d::ZeroRotator;

	switch (LagNode->AxisSpace)
	{
		case ECameraNodeSpace::CameraPose:
			AxesRotation = Result.CameraPose.GetRotation();
			break;

		case ECameraNodeSpace::World:
		default:
			break;
	}

	OutForward = AxesRotation.RotateVector(FVector3d::ForwardVector);
	OutRight   = AxesRotation.RotateVector(FVector3d::RightVector);
	OutUp      = AxesRotation.RotateVector(FVector3d::UpVector);
}

void FLagCameraNodeEvaluator::OnRun(
		const FCameraNodeEvaluationParams& Params,
		FCameraNodeEvaluationResult& OutResult)
{
	const FVector3d TargetPosition = OutResult.CameraPose.GetLocation();
	const ULagCameraNode* LagNode = GetCameraNodeAs<ULagCameraNode>();

	// First frame or uninitialized — snap.
	if (Params.bIsFirstFrame || !bIsInitialized)
	{
		LocalLag = FVector3d::ZeroVector;
		PreviousTargetPosition = TargetPosition;
		bIsInitialized = true;
		return;
	}

	// Camera cut handling.
	if (OutResult.bIsCameraCut)
	{
		if (LagNode->bSnapOnCameraCut)
		{
			LocalLag = FVector3d::ZeroVector;
			PreviousTargetPosition = TargetPosition;
		}
	}

	// Teleport snap — reset local lag entirely.
	const float MaxSnapDistance = MaxSnapDistanceReader.Get(OutResult.VariableTable);
	if (MaxSnapDistance > 0.f)
	{
		// Convert local lag to world-space distance for the check.
		FVector3d Forward, Right, Up;
		GetAxes(OutResult, Forward, Right, Up);
		const FVector3d WorldLag = Forward * LocalLag.X + Right * LocalLag.Y + Up * LocalLag.Z;
		if (WorldLag.Length() > MaxSnapDistance)
		{
			LocalLag = FVector3d::ZeroVector;
			PreviousTargetPosition = TargetPosition;
		}
	}

	if (!OutResult.bIsCameraCut)
	{
		// Get reference axes for this frame.
		FVector3d Forward, Right, Up;
		GetAxes(OutResult, Forward, Right, Up);

		// Compute how much the target moved in the reference frame since last frame.
		const FVector3d TargetDelta = TargetPosition - PreviousTargetPosition;
		const double ForwardMove = TargetDelta.Dot(Forward);
		const double LateralMove  = TargetDelta.Dot(Right);
		const double VerticalMove = TargetDelta.Dot(Up);

		// Lag grows by however much the target "ran away" from us.
		LocalLag.X += ForwardMove;
		LocalLag.Y += LateralMove;
		LocalLag.Z += VerticalMove;

		// Each component independently catches up toward zero.
		const float ForwardLagSpeed = ForwardLagSpeedReader.Get(OutResult.VariableTable);
		const float LateralLagSpeed = LateralLagSpeedReader.Get(OutResult.VariableTable);
		const float VerticalLagSpeed = VerticalLagSpeedReader.Get(OutResult.VariableTable);

		const float ForwardAlpha = FMath::Clamp(ForwardLagSpeed * Params.DeltaTime, 0.f, 1.f);
		const float LateralAlpha  = FMath::Clamp(LateralLagSpeed  * Params.DeltaTime, 0.f, 1.f);
		const float VerticalAlpha = FMath::Clamp(VerticalLagSpeed * Params.DeltaTime, 0.f, 1.f);

		LocalLag.X *= (1.0 - ForwardAlpha);
		LocalLag.Y *= (1.0 - LateralAlpha);
		LocalLag.Z *= (1.0 - VerticalAlpha);

		PreviousTargetPosition = TargetPosition;
	}

	// Convert local lag back to world space and apply.
	FVector3d Forward, Right, Up;
	GetAxes(OutResult, Forward, Right, Up);
	const FVector3d WorldLag = Forward * LocalLag.X + Right * LocalLag.Y + Up * LocalLag.Z;
	OutResult.CameraPose.SetLocation(TargetPosition - WorldLag);
}

void FLagCameraNodeEvaluator::OnSerialize(
		const FCameraNodeEvaluatorSerializeParams& Params,
		FArchive& Ar)
{
	Super::OnSerialize(Params, Ar);

	Ar << LocalLag;
	Ar << PreviousTargetPosition;
	Ar << bIsInitialized;
}

}  // namespace UE::Cameras

FCameraNodeEvaluatorPtr ULagCameraNode::OnBuildEvaluator(FCameraNodeEvaluatorBuilder& Builder) const
{
	using namespace UE::Cameras;
	return Builder.BuildEvaluator<FLagCameraNodeEvaluator>();
}
