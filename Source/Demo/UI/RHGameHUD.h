#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "Demo/UI/RHPlayerHUDWidget.h"
#include "RHGameHUD.generated.h"

class APawn;

UCLASS()
class DEMO_API ARHGameHUD : public AHUD
{
	GENERATED_BODY()

public:
	ARHGameHUD();

	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD")
	TSubclassOf<URHPlayerHUDWidget> PlayerHUDClass;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "HUD")
	TObjectPtr<URHPlayerHUDWidget> PlayerHUDWidget;

protected:
	UFUNCTION()
	void HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn);
};
