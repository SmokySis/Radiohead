#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "Demo/Character/BaseCharacter.h"
#include "Demo/Combat/RHCombatActionInterface.h"
#include "Demo/Combat/RHCombatComponent.h"
#include "Demo/Component/RHEquipComponent.h"
#include "Demo/Component/KnsTargetLockComponent.h"
#include "Demo/GAS/KnsAbilitySystemComponent.h"
#include "Demo/GAS/KnsCommonAttributeSet.h"
#include "Demo/Onom/RHOnomComponent.h"
#include "SLCharacterBase.generated.h"

class UAnimMontage;

UCLASS()
class DEMO_API ASLCharacterBase : public ABaseCharacter, public IRHCombatActionInterface
{
	GENERATED_BODY()

public:
	ASLCharacterBase();

	virtual void BeginPlay() override;
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual void SetComboWindowOpen(bool bOpen) override;
	virtual void SetPreInputWindowOpen(bool bOpen) override;
	virtual bool HasPendingAction() const override;
	virtual bool CanConsumeOnom(int32 Amount) const override;
	virtual bool TryConsumeOnom(int32 Amount) override;
	virtual void StartExecution(AActor* Enemy) override;
	virtual void HandleDeflected(AActor* Enemy) override;
	virtual void NotifyEnemyDefeated(AActor* Enemy) override;
	virtual void SetExecutionAvailable(bool bAvailable, AActor* Enemy) override;
	virtual bool TryStartExecution() override;

	/** True when the current move input points roughly opposite to the character's facing (quick-turn condition). */
	UFUNCTION(BlueprintPure, Category = "SL|Movement")
	bool IsMoveInputOppositeFacing() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SL|GAS")
	TObjectPtr<UKnsAbilitySystemComponent> SLAbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SL|GAS")
	TObjectPtr<UKnsCommonAttributeSet> SLCommonAttributeSet;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RH|Combat")
	TObjectPtr<URHCombatComponent> RHCombatComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SL|Lock")
	TObjectPtr<UKnsTargetLockComponent> SLTargetLockComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RH|Onom")
	TObjectPtr<URHOnomComponent> RHOnomComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RH|Equip")
	TObjectPtr<URHEquipComponent> RHEquipComponent;

protected:
	UFUNCTION()
	void HandleExecutionMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	UFUNCTION()
	void HandleLockIndicatorChanged(AActor* Target);

	/** 锁定解除：只隐藏旧目标的白点/血条，不重新点亮（OnLockReleased 传的是被释放的目标）。 */
	UFUNCTION()
	void HandleLockReleased(AActor* Target);

	/** 当前是否处于某个敌人的处决范围内。 */
	UPROPERTY(Transient)
	bool bExecutionAvailable = false;

	UPROPERTY(Transient)
	TObjectPtr<AActor> ExecutionEnemy;

	UPROPERTY(Transient)
	TObjectPtr<AActor> LockIndicatorTarget;

	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> ActiveExecutionMontage;

};
