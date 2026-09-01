#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "Demo/Combat/RHCombatComponent.h"
#include "RHCancelNotify.generated.h"

/**
 * RH Cancel（取消/插入开放点，时间点 Notify）：
 * 从该帧起，对应行为可插入打断当前动作（tag 持续到动作结束，由战斗组件统一清除）。
 * 支持一次勾选多个类型（位掩码）：Roll / Move / Attack / Special / Defensive / Other。
 */
UCLASS(meta = (DisplayName = "RH Cancel"))
class DEMO_API URHCancelNotify : public UAnimNotify
{
	GENERATED_BODY()

public:
	/** 勾选哪些类型，通知就打开哪些取消窗口（默认 Special）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cancel", meta = (Bitmask, BitmaskEnum = "/Script/Demo.ERHCancelType"))
	int32 CancelTypes = static_cast<int32>(ERHCancelType::Special);

	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};
