#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Demo/Onom/RHOnomTypes.h"
#include "RHDodgeCoreDefinition.generated.h"

/**
 * 闪避成功效果配置（Dodge Core DA）：
 * 闪避成功时按数组依次执行效果（共鸣时长/音形增减/触发 just load/触发弹反等）。
 * 时间变慢、mesh 覆盖材质、相机 rig 等固定反馈不在这里配置（走战斗组件 RH|Dodge）。
 */
UCLASS(BlueprintType)
class DEMO_API URHDodgeCoreDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dodge")
	TArray<FRHOnomEffect> DodgeEffects;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(TEXT("RHDodgeCoreDefinition"), GetFName());
	}
};
