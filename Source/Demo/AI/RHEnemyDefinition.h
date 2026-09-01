#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RHEnemyTypes.h"
#include "RHEnemyDefinition.generated.h"

class UAnimMontage;
class URHEnemyMoveDefinition;
class URHEnemyMovePoolDefinition;

/** 一个敌人一份配置：固有属性、招式池、反击条、共振值、状态蒙太奇、转阶段全部在这里（不区分小怪/Boss）。 */
UCLASS(BlueprintType)
class DEMO_API URHEnemyDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AssetRegistrySearchable, Category = "Enemy")
	FName EnemyId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Attributes")
	FRHEnemyAttributeConfig Attributes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|CounterBar", meta = (ToolTip = "隐藏 AI 参数，不进 UI"))
	FRHEnemyCounterBarConfig CounterBar;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Resonance")
	FRHEnemyResonanceConfig Resonance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Feel")
	FRHEnemyFeelConfig Feel;

	/** 近战招式池（起手式 + 中间招式段 + 收尾式 + 特殊招式，特殊只在收尾后派生）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Moves")
	TObjectPtr<URHEnemyMovePoolDefinition> MeleeMovePool;

	/** 远程招式池：直接引用普通招式（固定普攻、无特殊），随机触发；无可用招式则交给 approach。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Moves")
	TArray<TObjectPtr<URHEnemyMoveDefinition>> RemoteMoves;

	/** 玩家距离达到该值时优先尝试远程招式。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Moves", meta = (ClampMin = "0"))
	float RemoteAttackThreshold = 800.f;

	/** 远程招式冷却：用过一次远程后，该秒数内远程任务/Aggressive 远程分支直接 Failed 交给 approach。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Moves", meta = (ClampMin = "0"))
	float RemoteCooldownSeconds = 5.f;

	/** 勾选 = 小怪：血条显示在敌人自带 Widget 组件上；不勾选 = Boss：显示在玩家 UI。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|UI")
	bool bFloatBar = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Montages")
	TSoftObjectPtr<UAnimMontage> DeflectMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Montages")
	TSoftObjectPtr<UAnimMontage> DownMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Montages")
	TSoftObjectPtr<UAnimMontage> GetupMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Montages")
	TSoftObjectPtr<UAnimMontage> DeathMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Montages")
	TSoftObjectPtr<UAnimMontage> DodgeMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Montages")
	TSoftObjectPtr<UAnimMontage> GuardMontage;

	/** 转阶段直接写进敌人 DA。只填转阶段条目（如 66%/33%）；初始阶段（100%）就是上方的属性/招式配置，不用单独建一项。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Phases")
	TArray<FRHEnemyPhaseConfig> Phases;

	UFUNCTION(BlueprintPure, Category = "Enemy")
	int32 GetPhaseCount() const { return Phases.Num() + 1; }

	/** 解析 基础 + 阶段 Override 后的完整运行时配置。PhaseIndex 0 = 基础。 */
	void GetResolvedConfig(int32 PhaseIndex, FRHEnemyRuntimeConfig& OutConfig) const;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
};
