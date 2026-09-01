#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "RHEnemyAutoRotateNotify.generated.h"

/**
 * Enemy Auto Rotate（敌人自动转向开关，时间点 Notify）。
 * 放在敌人攻击蒙太奇里控制"旋转至玩家"：
 *   open=true  → 启用旋转（每段攻击开始默认已启用，任务层会重新开）；
 *   open=false → 禁用旋转（连招中途固定朝向、转身等场景，不再追踪玩家）。
 * 实际调用 RHEnemyAIComponent::SetRotateToPlayer。
 */
UCLASS(meta = (DisplayName = "Enemy Auto Rotate"))
class DEMO_API URHEnemyAutoRotateNotify : public UAnimNotify
{
	GENERATED_BODY()

public:
	/** true = 启用旋转至玩家；false = 禁用。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy", meta = (DisplayName = "Open"))
	bool bOpen = true;

	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};
