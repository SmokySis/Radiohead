#pragma once
#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "Demo/Combat/RHHitData.h"
#include "KnsAbilitySystemComponent.generated.h"

class AActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FKnsActorDied, AActor*, Actor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FKnsResonanceBroken, AActor*, Actor);

UCLASS()
class DEMO_API UKnsAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	UKnsAbilitySystemComponent();

	UFUNCTION(BlueprintPure, Category = "Kns|Attributes", meta = (ToolTip = "读取任意属性的当前值"))
	float GetAttributeValue(const FGameplayAttribute& Attribute) const;

	UFUNCTION(BlueprintPure, Category = "Kns|Stamina", meta = (ToolTip = "当前体力"))
	float GetStamina() const;

	UFUNCTION(BlueprintPure, Category = "Kns|Stamina", meta = (ToolTip = "体力上限"))
	float GetMaxStamina() const;

	UFUNCTION(BlueprintPure, Category = "Kns|Stamina", meta = (ToolTip = "体力是否足够"))
	bool HasEnoughStamina(float Amount) const;

	UFUNCTION(BlueprintCallable, Category = "Kns|Stamina", meta = (ToolTip = "尝试消耗体力，不足时返回 false"))
	bool TryConsumeStamina(float Amount);

	UFUNCTION(BlueprintCallable, Category = "Kns|Stamina", meta = (ToolTip = "回复体力，自动截断到上限"))
	void RegenerateStamina(float Amount);

	UFUNCTION(BlueprintPure, Category = "Kns|Onom", meta = (ToolTip = "当前 Onom 值"))
	float GetOnom() const;

	UFUNCTION(BlueprintPure, Category = "Kns|Onom", meta = (ToolTip = "Onom 上限"))
	float GetMaxOnom() const;

	UFUNCTION(BlueprintPure, Category = "Kns|Onom", meta = (ToolTip = "Onom 是否足够"))
	bool HasEnoughOnom(float Amount) const;

	UFUNCTION(BlueprintCallable, Category = "Kns|Onom", meta = (ToolTip = "尝试消耗 Onom，不足时返回 false"))
	bool TryConsumeOnom(float Amount);

	UFUNCTION(BlueprintCallable, Category = "Kns|Onom", meta = (ToolTip = "回复 Onom，自动截断到上限"))
	void RegenerateOnom(float Amount);

	UFUNCTION(BlueprintPure, Category = "Kns|Focus", meta = (ToolTip = "当前 Focus 充能"))
	float GetFocus() const;

	UFUNCTION(BlueprintPure, Category = "Kns|Focus", meta = (ToolTip = "Focus 上限"))
	float GetMaxFocus() const;

	UFUNCTION(BlueprintPure, Category = "Kns|Focus", meta = (ToolTip = "Focus 是否足够"))
	bool HasEnoughFocus(float Amount) const;

	UFUNCTION(BlueprintCallable, Category = "Kns|Focus", meta = (ToolTip = "尝试消耗 Focus，不足时返回 false"))
	bool TryConsumeFocus(float Amount);

	UFUNCTION(BlueprintCallable, Category = "Kns|Focus", meta = (ToolTip = "回复 Focus，自动截断到上限"))
	void RegenerateFocus(float Amount);

	UPROPERTY(BlueprintAssignable, Category = "Kns|Death")
	FKnsActorDied OnActorDied;

	UFUNCTION(BlueprintCallable, Category = "Kns|Damage", meta = (ToolTip = "对目标应用伤害 GE，MoveMultiplier 是招式倍率，CritMultiplier 是暴击倍率"))
	void ApplyDamageToActor(AActor* TargetActor, float MoveMultiplier = 1.f, float CritMultiplier = 1.5f);

	/** Unified flat-damage entry (big break etc.): resolves via Damage.Flat without Onom penalty. */
	UFUNCTION(BlueprintCallable, Category = "Kns|Damage", meta = (ToolTip = "Unified flat damage entry, no Onom penalty"))
	void ApplyFlatDamageToActor(AActor* TargetActor, float Damage);

	UFUNCTION(BlueprintCallable, Category = "Kns|Damage", meta = (ToolTip = "统一命中入口：伤害、共振伤害、防御系数与破防事件"))
	bool ApplyHitToActor(AActor* TargetActor, const FRHHitData& HitData);

	UFUNCTION(BlueprintCallable, Category = "Kns|Poise", meta = (ToolTip = "对目标施加削韧值"))
	void ApplyPoiseDamageToActor(AActor* TargetActor, float Amount);

	UFUNCTION(BlueprintPure, Category = "Kns|Resonance", meta = (ToolTip = "当前共振值"))
	float GetResonance() const;

	UFUNCTION(BlueprintPure, Category = "Kns|Resonance", meta = (ToolTip = "共振值上限"))
	float GetMaxResonance() const;

	UFUNCTION(BlueprintCallable, Category = "Kns|Resonance", meta = (ToolTip = "回复共振值，自动截断到上限"))
	void RegenerateResonance(float Amount);

	UFUNCTION(BlueprintCallable, Category = "Kns|Resonance", meta = (ToolTip = "把共振值重置到上限"))
	void ResetResonance();

	UFUNCTION(BlueprintCallable, Category = "Kns|Resonance", meta = (ToolTip = "按增量修改共振值（可正可负），自动截断到 [0, 上限]"))
	void ApplyResonanceDelta(float Amount);

	UPROPERTY(BlueprintAssignable, Category = "Kns|Resonance")
	FKnsResonanceBroken OnResonanceBroken;

protected:
	void ApplyResourceChange(FName ResourceName, float Amount);
	void ApplyResourceChangeToActor(AActor* TargetActor, FName ResourceName, float Amount);
};

