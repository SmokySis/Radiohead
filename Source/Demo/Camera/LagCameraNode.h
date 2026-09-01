// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/CameraNode.h"
#include "Core/CameraParameters.h"
#include "Nodes/CameraNodeTypes.h"

#include "LagCameraNode.generated.h"

/**
 * A camera node that applies traditional world-space position lag,
 * similar to SpringArmComponent's camera lag. The camera smoothly
 * follows its target position with configurable per-axis speed.
 *
 * Algorithm: lag is stored in reference-local space as (forward, lateral,
 * vertical) components. When the reference rotation changes (character
 * turns), the local components naturally rotate with it — no snapping,
 * no stale per-axis state.
 *
 * IMPORTANT: place this node BEFORE any offset or rotation nodes.
 * Pipeline: [Rig] -> [Lag] -> [Offset] -> ...
 */
UCLASS(MinimalAPI, meta=(DisplayName="Camera Lag", CameraNodeCategories="Common,Transform"))
class ULagCameraNode : public UCameraNode
{
	GENERATED_BODY()

protected:

	// UCameraNode interface.
	virtual FCameraNodeEvaluatorPtr OnBuildEvaluator(FCameraNodeEvaluatorBuilder& Builder) const override;

public:

	/** Lag speed along the forward axis of the reference space. */
	UPROPERTY(EditAnywhere, Category="Lag|Speed", meta=(ClampMin="0"))
	FFloatCameraParameter ForwardLagSpeed = 10.f;

	/** Lag speed along the right axis of the reference space. */
	UPROPERTY(EditAnywhere, Category="Lag|Speed", meta=(ClampMin="0"))
	FFloatCameraParameter LateralLagSpeed = 10.f;

	/** Lag speed along the up axis of the reference space. */
	UPROPERTY(EditAnywhere, Category="Lag|Speed", meta=(ClampMin="0"))
	FFloatCameraParameter VerticalLagSpeed = 10.f;

	/**
	 * Which rotation drives the forward/lateral/vertical decomposition.
	 * CameraPose (default): use current camera rotation — correct when
	 *   Lag is placed before any offset/rotation nodes.
	 * World: world axes. Equivalent to all three speeds being equal.
	 */
	UPROPERTY(EditAnywhere, Category="Lag")
	ECameraNodeSpace AxisSpace = ECameraNodeSpace::CameraPose;

	/**
	 * If non-zero, the camera snaps immediately when distance to target
	 * exceeds this value. Prevents slow drift after teleports.
	 */
	UPROPERTY(EditAnywhere, Category="Lag|Snap", meta=(ClampMin="0"))
	FFloatCameraParameter MaxSnapDistance = 0.f;

	/** Whether to snap immediately on a camera cut. */
	UPROPERTY(EditAnywhere, Category="Lag|Snap")
	bool bSnapOnCameraCut = true;
};
