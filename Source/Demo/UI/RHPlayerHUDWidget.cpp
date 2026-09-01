#include "RHPlayerHUDWidget.h"

#include "Demo/Component/KnsTargetLockComponent.h"
#include "Demo/Character/RHEnemyBase.h"
#include "Demo/Onom/RHOnomComponent.h"
#include "GameFramework/Pawn.h"

void URHPlayerHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void URHPlayerHUDWidget::BindPlayer(APawn* InPawn)
{
	if (!InPawn || BoundPlayer.Get() == InPawn)
	{
		return;
	}

	BoundPlayer = InPawn;

	if (PlayerPanel)
	{
		PlayerPanel->BindHealthSource(InPawn);
	}

	if (URHOnomComponent* Onom = InPawn->FindComponentByClass<URHOnomComponent>())
	{
		if (PlayerPanel)
		{
			PlayerPanel->BindOnomComponent(Onom);
		}
	}

	if (UKnsTargetLockComponent* Lock = InPawn->FindComponentByClass<UKnsTargetLockComponent>())
	{
		Lock->OnLockAcquired.AddDynamic(this, &URHPlayerHUDWidget::HandleLockAcquired);
		Lock->OnLockReleased.AddDynamic(this, &URHPlayerHUDWidget::HandleLockReleased);
		Lock->OnTargetSwitched.AddDynamic(this, &URHPlayerHUDWidget::HandleLockAcquired);
		BindEnemy(Lock->GetLockedTarget());
	}
}

void URHPlayerHUDWidget::BindEnemy(AActor* InEnemy)
{
	if (EnemyPanel)
	{
		AActor* Target = InEnemy;
		if (ARHEnemyBase* Enemy = Cast<ARHEnemyBase>(InEnemy))
		{
			if (Enemy->IsUsingFloatBar())
			{
				Target = nullptr;
			}
		}
		EnemyPanel->BindTarget(Target);
	}
}

void URHPlayerHUDWidget::HandleLockAcquired(AActor* Target)
{
	BindEnemy(Target);
}

void URHPlayerHUDWidget::HandleLockReleased(AActor* PreviousTarget)
{
	BindEnemy(nullptr);
}
