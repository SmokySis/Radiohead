#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "KnsMoveRotationAdjustNotifyState.generated.h"

class UInputAction;

/**
 * Kns Move Rotation Adjust（朝向随输入旋转修正）
 * 所属系统：KNS（MH 遗留）
 * 驱动组件：角色自身（NotifyTick 按移动输入旋转）
 * 用途：出招期间按移动输入方向旋转角色朝向（可限总转角）
 * 放置：出招期间需要转向的招式段
 * 参数：MoveInputAction / DeadZone / RotationSpeed / RotationCoefficient / MaxTotalYaw / bDrawDebug
 */
UCLASS(meta = (DisplayName = "Kns Move Rotation Adjust"))
class DEMO_API UKnsMoveRotationAdjustNotifyState : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;

	// 读取移动输入用的 IA_Move，需要 Axis2D
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input", meta = (ToolTip = "读取移动输入用的 IA_Move，需要 Axis2D"))
	TObjectPtr<UInputAction> MoveInputAction;

	// 小于该幅度的输入不参与微调
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input", meta = (ClampMin = "0.0", ToolTip = "小于该幅度的输入不参与微调"))
	float DeadZone = 0.25f;

	// 每秒最大旋转角度
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rotation", meta = (ClampMin = "0.0", ToolTip = "每秒最大旋转角度"))
	float RotationSpeed = 30.f;

	// 输入力度系数，0~1，摇杆推得越满转得越快
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rotation", meta = (ClampMin = "0.0", ClampMax = "1.0", ToolTip = "输入力度系数，0~1，摇杆推得越满转得越快"))
	float RotationCoefficient = 0.5f;

	// 这段 ANS 内允许累计的最大转向角度，0 表示不限制
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rotation", meta = (ClampMin = "0.0", ToolTip = "这段 ANS 内允许累计的最大转向角度，0 表示不限制"))
	float MaxTotalYaw = 15.f;

	// 是否绘制移动输入方向箭头
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug", meta = (ToolTip = "是否绘制移动输入方向箭头"))
	bool bDrawDebug = true;

private:
	float AccumulatedYaw = 0.f;
};
