#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "KnsResourceRegenComponent.generated.h"

class UKnsAbilitySystemComponent;

UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class DEMO_API UKnsResourceRegenComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UKnsResourceRegenComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// 是否自动回复体力
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Regen|Stamina", meta = (ToolTip = "是否自动回复体力"))
	bool bAutoRegenStamina = true;

	// 体力下降后延迟多久开始回复
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Regen|Stamina", meta = (ClampMin = "0.0", ToolTip = "体力下降后延迟多久开始回复"))
	float StaminaRegenDelay = 1.f;

	// 每秒回复的体力值
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Regen|Stamina", meta = (ClampMin = "0.0", ToolTip = "每秒回复的体力值"))
	float StaminaRegenRate = 20.f;

	// Whether Onom should auto-regen after a delay.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Regen|Onom", meta = (ToolTip = "Whether Onom should auto-regen after a delay"))
	bool bAutoRegenOnom = false;

	// Delay before Onom regen starts.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Regen|Onom", meta = (ClampMin = "0.0", ToolTip = "Delay before Onom regen starts"))
	float OnomRegenDelay = 1.f;

	// Onom regen per second.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Regen|Onom", meta = (ClampMin = "0.0", ToolTip = "Onom regen per second"))
	float OnomRegenRate = 0.f;

	// Whether Focus should auto-charge after a delay.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Regen|Focus", meta = (ToolTip = "Whether Focus should auto-charge after a delay"))
	bool bAutoRegenFocus = false;

	// Delay before Focus regen starts.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Regen|Focus", meta = (ClampMin = "0.0", ToolTip = "Delay before Focus regen starts"))
	float FocusRegenDelay = 1.f;

	// Focus regen per second.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Regen|Focus", meta = (ClampMin = "0.0", ToolTip = "Focus regen per second"))
	float FocusRegenRate = 0.f;

protected:
	UKnsAbilitySystemComponent* GetAbilitySystemComponent() const;

	UPROPERTY(Transient)
	float LastStaminaValue = 0.f;

	UPROPERTY(Transient)
	float LastStaminaChangeTime = 0.f;

	UPROPERTY(Transient)
	float LastOnomValue = 0.f;

	UPROPERTY(Transient)
	float LastOnomChangeTime = 0.f;

	UPROPERTY(Transient)
	float LastFocusValue = 0.f;

	UPROPERTY(Transient)
	float LastFocusChangeTime = 0.f;
};
