#include "RHEnemyAIController.h"

#include "Components/StateTreeAIComponent.h"
#include "Demo/Character/RHEnemyBase.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"

ARHEnemyAIController::ARHEnemyAIController()
{
	StateTreeComponent = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("StateTreeAI"));
	bStartAILogicOnPossess = true;
}

void ARHEnemyAIController::OnPossess(APawn* InPawn)
{
	// 在 Super（会触发 StartLogic）之前把敌人 BP 配的资产喂给状态树组件。
	if (ARHEnemyBase* Enemy = Cast<ARHEnemyBase>(InPawn))
	{
		if (UStateTree* Asset = Enemy->EnemyStateTreeAsset)
		{
			if (StateTreeComponent)
			{
				StateTreeComponent->SetStateTree(Asset);
			}
		}
	}

	Super::OnPossess(InPawn);
}

void ARHEnemyAIController::RespawnEnemyPawn(const FTransform& InTransform)
{
	UWorld* World = GetWorld();
	APawn* CurrentPawn = GetPawn();
	if (!World || !CurrentPawn)
	{
		return;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	APawn* NewPawn = World->SpawnActor<APawn>(CurrentPawn->GetClass(), InTransform, Params);
	if (!NewPawn)
	{
		return;
	}

	// 下一帧再 Possess：避免在状态树执行（Tick）中重入 StartLogic。
	World->GetTimerManager().SetTimerForNextTick([this, NewPawn, CurrentPawn]()
	{
		if (IsValid(this) && IsValid(NewPawn))
		{
			Possess(NewPawn);
			if (IsValid(CurrentPawn))
			{
				CurrentPawn->Destroy();
			}
		}
	});
}
