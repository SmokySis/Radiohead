#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"
#include "Demo/Onom/RHOnomTypes.h"
#include "RHHitFeedbackDefinition.generated.h"

/** 按极性区分的命中反馈条目：只含 Niagara 引用（可选音效），无其他参数。 */
USTRUCT(BlueprintType)
struct FRHHitFeedbackPolarityEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Feedback")
	ERHOnomPolarity Polarity = ERHOnomPolarity::Major;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Feedback")
	TObjectPtr<USoundBase> Sound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Feedback")
	TObjectPtr<UNiagaraSystem> VFX;
};

/** 命中/防御反馈，挂在战斗组件下统一配置。 */
UCLASS(BlueprintType)
class DEMO_API URHHitFeedbackDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** 普攻命中：按本次会获得的 Onom 极性选档。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
	TArray<FRHHitFeedbackPolarityEntry> AttackHitFeedback;

	/** 战技/音律武器命中：按本次消耗的极性选档。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
	TArray<FRHHitFeedbackPolarityEntry> SkillHitFeedback;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard")
	TObjectPtr<USoundBase> GuardSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard")
	TObjectPtr<UNiagaraSystem> GuardVFX;

	/** 完美防御/弹反：固定音效。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard")
	TObjectPtr<USoundBase> ParrySound;

	/** 完美防御/弹反：按本次获得的 Onom 极性选特效。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard")
	TArray<FRHHitFeedbackPolarityEntry> ParryFeedback;

	/** 相杀成功：固定音效，不按极性变化。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clash")
	TObjectPtr<USoundBase> ClashSound;

	/** 相杀成功：固定特效，不按极性变化。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clash")
	TObjectPtr<UNiagaraSystem> ClashVFX;

	UFUNCTION(BlueprintPure, Category = "Feedback")
	bool GetEntry(const TArray<FRHHitFeedbackPolarityEntry>& Entries, ERHOnomPolarity Polarity, FRHHitFeedbackPolarityEntry& OutEntry) const;
};
