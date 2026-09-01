#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/ProgressBar.h"
#include "RHEnemyFloatPanelWidget.generated.h"

class UKnsAbilitySystemComponent;
struct FOnAttributeChangeData;

/** 敌人自带血条专用：可蓝图子类化。蓝图里必须放两个 bar，命名 EnemyHPBar / EnemyResonanceBar。 */
UCLASS(Blueprintable)
class DEMO_API URHEnemyFloatPanelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Enemy|UI")
	void BindTarget(AActor* InTarget);

protected:
	virtual void NativeConstruct() override;

	void HandleHealthChanged(const FOnAttributeChangeData& Data);
	void HandleResonanceChanged(const FOnAttributeChangeData& Data);

	void RefreshBars();

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> EnemyHPBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> EnemyResonanceBar;

private:
	UPROPERTY(Transient)
	TObjectPtr<UKnsAbilitySystemComponent> BoundASC;
};
