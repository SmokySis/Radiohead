#include "RHTrailNotifyState.h"

#include "Components/SkeletalMeshComponent.h"

// 武器拖尾已改为代码全权管理（动作生命周期自动开关）：
// - 玩家：RHCombatComponent::StartAction 释放时 ActivateCastTrail()，HandleActionMontageEnded/CancelAction 关闭；
// - 敌人：RHEnemyCombatComponent::PlayAction 施放时 ActivateCastTrail()，动作结束/停止时 DeactivateCastTrail()。
// 本 ANS 保留仅为兼容存量蒙太奇上的通知（删除会导致资产引用失效），不再控制拖尾。
void URHTrailNotifyState::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
}

void URHTrailNotifyState::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);
}
