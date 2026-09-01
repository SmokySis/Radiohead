#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "KnsCombatContextComponent.generated.h"

class UKnsMoveDefinition;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FKnsContextCancelRequested);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FKnsContextHitConditionRequested, FGameplayTag, Tag);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FKnsContextMoveChanged, UKnsMoveDefinition*, Move, FName, NodeId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FKnsContextComboActiveChanged, bool, bActive);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FKnsContextPoiseChanged, int32, BasePoise, int32, EffectivePoise);

UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class DEMO_API UKnsCombatContextComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UKnsCombatContextComponent();

	UPROPERTY(BlueprintAssignable, Category = "CombatContext|Events", meta = (ToolTip = "请求取消当前连招"))
	FKnsContextCancelRequested OnCancelRequested;

	UPROPERTY(BlueprintAssignable, Category = "CombatContext|Events", meta = (ToolTip = "请求挂一个命中条件 Tag"))
	FKnsContextHitConditionRequested OnHitConditionTagRequested;

	UPROPERTY(BlueprintAssignable, Category = "CombatContext|Events", meta = (ToolTip = "当前招式变化"))
	FKnsContextMoveChanged OnMoveChanged;

	UPROPERTY(BlueprintAssignable, Category = "CombatContext|Events", meta = (ToolTip = "连招激活状态变化"))
	FKnsContextComboActiveChanged OnComboActiveChanged;

	UPROPERTY(BlueprintAssignable, Category = "CombatContext|Events", meta = (ToolTip = "韧性等级变化"))
	FKnsContextPoiseChanged OnPoiseChanged;

	UFUNCTION(BlueprintCallable, Category = "CombatContext", meta = (ToolTip = "请求取消当前连招"))
	void RequestCancelCombo();

	UFUNCTION(BlueprintCallable, Category = "CombatContext", meta = (ToolTip = "请求挂一个命中条件 Tag"))
	void RequestHitConditionTag(FGameplayTag Tag);

	UFUNCTION(BlueprintCallable, Category = "CombatContext", meta = (ToolTip = "写入当前招式状态"))
	void SetMoveState(UKnsMoveDefinition* Move, FName NodeId, int32 BasePoise, int32 EffectivePoise, bool bActive, float Resistance = 0.f);

	UFUNCTION(BlueprintCallable, Category = "CombatContext", meta = (ToolTip = "写入韧性状态"))
	void SetPoiseState(int32 BasePoise, int32 EffectivePoise, float Resistance = 0.f);

	UFUNCTION(BlueprintCallable, Category = "CombatContext", meta = (ToolTip = "清空战斗状态"))
	void ClearCombatState();

	UFUNCTION(BlueprintPure, Category = "CombatContext", meta = (ToolTip = "当前招式"))
	UKnsMoveDefinition* GetCurrentMove() const;

	UFUNCTION(BlueprintPure, Category = "CombatContext", meta = (ToolTip = "当前有效韧性"))
	int32 GetEffectivePoiseLevel() const;

	/** 当前招式的减伤系数（执行中被命中时生效，独立于打断判定）。 */
	UFUNCTION(BlueprintPure, Category = "CombatContext", meta = (ToolTip = "Current move damage reduction"))
	float GetResistance() const;

	UFUNCTION(BlueprintPure, Category = "CombatContext", meta = (ToolTip = "是否处于连招中"))
	bool IsComboActive() const;

protected:
	UPROPERTY(Transient)
	TObjectPtr<UKnsMoveDefinition> CurrentMove;

	UPROPERTY(Transient)
	FName CurrentNodeId;

	UPROPERTY(Transient)
	int32 BasePoiseLevel = 0;

	UPROPERTY(Transient)
	int32 EffectivePoiseLevel = 0;

	UPROPERTY(Transient)
	float Resistance = 0.f;

	UPROPERTY(Transient)
	bool bComboActive = false;
};
