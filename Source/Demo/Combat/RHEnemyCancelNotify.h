#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "RHEnemyCancelNotify.generated.h"

/**
 * 敌人取消窗口（单点 AnimNotify）：
 * 放在敌人蒙太奇允许取消的帧上，触发时发 AI.Enemy.Cancel 事件让状态树重选。
 * 敌人不需要玩家那套 Roll/Move/Attack/Special/Defensive 分类，一个 cancel 就是"可以重新选任务"。
 */
UCLASS(meta = (DisplayName = "Enemy Cancel"))
class DEMO_API URHEnemyCancelNotify : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;
};
