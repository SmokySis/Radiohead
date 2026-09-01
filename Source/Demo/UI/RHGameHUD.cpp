#include "RHGameHUD.h"

#include "Demo/UI/RHPlayerHUDWidget.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

ARHGameHUD::ARHGameHUD()
{
}

void ARHGameHUD::BeginPlay()
{
	Super::BeginPlay();

	if (PlayerHUDClass && !PlayerHUDWidget)
	{
		if (APlayerController* PC = GetOwningPlayerController())
		{
			PlayerHUDWidget = CreateWidget<URHPlayerHUDWidget>(PC, PlayerHUDClass);
			if (PlayerHUDWidget)
			{
				PlayerHUDWidget->AddToViewport();
			}

			PC->OnPossessedPawnChanged.AddDynamic(this, &ARHGameHUD::HandlePossessedPawnChanged);
			HandlePossessedPawnChanged(nullptr, PC->GetPawn());
		}
	}
}

void ARHGameHUD::HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn)
{
	if (PlayerHUDWidget)
	{
		PlayerHUDWidget->BindPlayer(NewPawn);
	}
}
