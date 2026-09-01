#pragma once

#include "CoreMinimal.h"
#include "Demo/Character/BaseCharacter.h"
#include "Demo/Combat/RHTargetLockInterface.h"
#include "Demo/GAS/KnsAbilitySystemComponent.h"
#include "Demo/GAS/KnsCommonAttributeSet.h"
#include "Demo/Onom/RHOnomTypes.h"
#include "RHEnemyBase.generated.h"

class ARHEnemyAIController;
class UKnsCombatContextComponent;
class UStateTree;
class URHEnemyAIComponent;
class URHEnemyCombatComponent;
class USphereComponent;
class UWidgetComponent;

/**
 * 敌人基类：GAS（ASC/CommonAS）+ 战斗上下文 + 敌人战斗组件 + 敌人 AI 组件 + StateTree 组件。
 * 数据（属性/招式/共振/反击条/转阶段）全部来自 URHEnemyDefinition，由 URHEnemyAIComponent 应用。
 */
UCLASS()
class DEMO_API ARHEnemyBase : public ABaseCharacter, public IRHTargetLockInterface
{
	GENERATED_BODY()

public:
	ARHEnemyBase();

	virtual void BeginPlay() override;
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	/** 开启/关闭处决范围检测（Down 期间开启）。 */
	void SetExecutionRangeActive(bool bActive);
	virtual void NotifyTargetLockChanged(bool bLocked) override;
	void SetLockIndicatorVisible(bool bVisible);
	/** 属性初始化完成后再次绑定/刷新自带血条，避免 0% 快照。 */
	void RefreshFloatPanel() { BindFloatBarWidget(); }
	void ApplyFloatBarConfig(bool bFloatBar);
	bool IsUsingFloatBar() const { return bFloatBar; }

	/** 死亡时隐藏身上全部 UI（锁定白点/浮层血条），防止死后残留。 */
	void HideAllUI();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RH|GAS")
	TObjectPtr<UKnsAbilitySystemComponent> RHAbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RH|GAS")
	TObjectPtr<UKnsCommonAttributeSet> RHCommonAttributeSet;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RH|Combat")
	TObjectPtr<UKnsCombatContextComponent> RHCombatContext;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RH|Combat")
	TObjectPtr<URHEnemyCombatComponent> RHEnemyCombatComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RH|AI")
	TObjectPtr<URHEnemyAIComponent> RHEnemyAIComponent;

	/** 敌人状态树资产：在敌人 BP 里指到 ST 资产，Possess 时由 ARHEnemyAIController 挂到 StateTreeAIComponent。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RH|AI")
	TObjectPtr<UStateTree> EnemyStateTreeAsset;

	/** 处决范围检测：进入 Down 时开启，玩家进入后由接口通知玩家可处决。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RH|Execution")
	TObjectPtr<USphereComponent> ExecutionRangeSphere;

	/** 锁定时显示在敌人位置的白点指示器。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RH|UI")
	TObjectPtr<UWidgetComponent> LockIndicatorWidget;

	/** 小怪自带血条组件（bFloatBar=true 时显示）。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RH|UI")
	TObjectPtr<UWidgetComponent> FloatingHealthBarWidget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RH|Onom")
	FRHOnomReactionOverride OnomReactionOverride;

protected:
	UPROPERTY(Transient)
	bool bFloatBar = false;

	UPROPERTY(Transient)
	bool bIsLocked = false;

	UPROPERTY(Transient)
	bool bFloatBarBindQueued = false;

	void BindFloatBarWidget();

	UFUNCTION()
	void OnExecutionOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnExecutionOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};
