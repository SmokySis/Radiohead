#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "Blueprint/UserWidget.h"
#include "Components/ProgressBar.h"
#include "GameplayEffectTypes.h"
#include "RHEnemyPanelWidget.generated.h"

class UKnsAbilitySystemComponent;

UCLASS()
class DEMO_API URHEnemyPanelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Enemy|UI")
	void BindTarget(AActor* InTarget);

	UFUNCTION(BlueprintCallable, Category = "Enemy|UI")
	void ClearTarget();

protected:
	virtual void NativeConstruct() override;

	void HandleHealthChanged(const FOnAttributeChangeData& Data);

	void HandleResonanceChanged(const FOnAttributeChangeData& Data);

	void RefreshBars();

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> EnemyHPBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> EnemyResonanceBar;

	UPROPERTY(Transient)
	TObjectPtr<UKnsAbilitySystemComponent> BoundASC;
};
