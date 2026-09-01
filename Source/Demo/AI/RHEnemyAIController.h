#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "RHEnemyAIController.generated.h"

class APawn;
class UStateTreeAIComponent;

/**
 * 敌人 AIController：挂引擎自带的 UStateTreeAIComponent（StateTree AI Component schema）。
 * Possess 时从敌人 Pawn 的 EnemyStateTreeAsset 取状态树资产并设置，随后自动 StartLogic。
 */
UCLASS()
class DEMO_API ARHEnemyAIController : public AAIController
{
	GENERATED_BODY()

public:
	ARHEnemyAIController();

	virtual void OnPossess(APawn* InPawn) override;

	/** 在死亡位置生成新敌人并（下一帧）重新 Possess，由死亡任务调用。 */
	void RespawnEnemyPawn(const FTransform& InTransform);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UStateTreeAIComponent> StateTreeComponent;
};
