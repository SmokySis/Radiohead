#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Sound/SoundBase.h"
#include "Demo/Onom/RHOnomTypes.h"
#include "RHCoreDefinition.generated.h"

UCLASS(BlueprintType)
class DEMO_API URHCoreDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
	URHCoreDefinition();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Core")
	FText DisplayName;

	/** 受伤时的音形结算模式：ClearAll（默认清空全部）或 ApplyRule（按下方规则失去/增加）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Core|Onom")
	ERHOnomDamageTakenMode DamageTakenMode = ERHOnomDamageTakenMode::ClearAll;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Core|Onom")
	FRHOnomSourceRule DamageTakenRule;

	/** 受击效果数组（替代写死的“清空+共鸣-2s”）：按顺序执行，数组为空时回落旧逻辑。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Core|Onom")
	TArray<FRHOnomEffect> DamageTakenEffects;
};
