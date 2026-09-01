#include "RHCancelNotify.h"

#include "Components/SkeletalMeshComponent.h"

void URHCancelNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (MeshComp && MeshComp->GetOwner())
	{
		if (URHCombatComponent* Combat = MeshComp->GetOwner()->FindComponentByClass<URHCombatComponent>())
		{
			const ERHCancelType Types[] = {
				ERHCancelType::Roll, ERHCancelType::Move, ERHCancelType::Attack,
				ERHCancelType::Special, ERHCancelType::Defensive, ERHCancelType::Other
			};
			for (const ERHCancelType Type : Types)
			{
				// 枚举值本身就是位值（Roll=1/Move=2/...），直接按位判断，不能 1<<Type。
				if (CancelTypes & static_cast<int32>(Type))
				{
					Combat->OpenCancel(Type);
				}
			}
		}
	}
}
