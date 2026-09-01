#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Demo/UI/RHEnemyPanelWidget.h"
#include "Demo/UI/RHPlayerPanelWidget.h"
#include "RHPlayerHUDWidget.generated.h"

class APawn;

UCLASS()
class DEMO_API URHPlayerHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void BindPlayer(APawn* InPawn);

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void BindEnemy(AActor* InEnemy);

protected:
	virtual void NativeConstruct() override;

	UFUNCTION()
	void HandleLockAcquired(AActor* Target);

	UFUNCTION()
	void HandleLockReleased(AActor* PreviousTarget);

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<URHPlayerPanelWidget> PlayerPanel;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<URHEnemyPanelWidget> EnemyPanel;

	UPROPERTY(Transient)
	TWeakObjectPtr<APawn> BoundPlayer;
};
