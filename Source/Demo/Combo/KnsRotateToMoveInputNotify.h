#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "KnsRotateToMoveInputNotify.generated.h"

class UInputAction;

/**
 * Kns Rotate To Move Input（转向移动输入，时间点 Notify）
 * 所属系统：KNS（MH 遗留）
 * 驱动组件：UKnsCombatComponent::StartRotateToMoveInput
 * 用途：出招时平滑转向当前移动输入方向（可选 Yaw 偏移）
 * 放置：需要转向的招式时间点
 * 参数：MoveInputAction / DeadZone / RotationOffsetYaw / RotationInterpSpeed / bDrawDebug / DebugDrawDuration
 */
UCLASS(meta = (DisplayName = "Kns Rotate To Move Input"))
class DEMO_API UKnsRotateToMoveInputNotify : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;

	// 读取当前移动输入用的 IA_Move，需要 Axis2D
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input", meta = (ToolTip = "读取当前移动输入用的 IA_Move，需要 Axis2D"))
	TObjectPtr<UInputAction> MoveInputAction;

	// 小于该幅度的输入不转向
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rotation", meta = (ClampMin = "0.0", ToolTip = "小于该幅度的输入不转向"))
	float DeadZone = 0.25f;

	// 转向目标方向后额外附加的 Yaw 偏移
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rotation", meta = (ToolTip = "转向目标方向后额外附加的 Yaw 偏移"))
	float RotationOffsetYaw = 0.f;

	// 转向插值速度，越大越快，0 表示瞬转
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rotation", meta = (ClampMin = "0.0", ToolTip = "转向插值速度，越大越快，0 表示瞬转"))
	float RotationInterpSpeed = 8.f;

	// 是否绘制目标方向与当前朝向的调试箭头
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug", meta = (ToolTip = "是否绘制目标方向与当前朝向的调试箭头"))
	bool bDrawDebug = true;

	// 调试箭头显示时长
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug", meta = (ClampMin = "0.0", ToolTip = "调试箭头显示时长"))
	float DebugDrawDuration = 0.2f;
};
