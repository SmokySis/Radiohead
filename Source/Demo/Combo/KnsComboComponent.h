#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "KnsComboTypes.h"
#include "KnsComboComponent.generated.h"

class AActor;
class UKnsCombatContextComponent;
class UKnsComboTreeData;
class UKnsMoveDefinition;
class UAbilitySystemComponent;
class UAnimMontage;
class USkeletalMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FKnsComboNodeStarted, UKnsComboTreeData*, Tree, FName, NodeId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FKnsComboEnded, bool, bCancelled);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FKnsMoveStarted, UKnsMoveDefinition*, Move, FName, NodeId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FKnsComboActiveChanged, bool, bActive);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FKnsPoiseChanged, int32, BasePoise, int32, EffectivePoise);

UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class DEMO_API UKnsComboComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UKnsComboComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// 把移动输入 Vector2D（X 左右，Y 前后，相对摄像机）换算成世界方向；无输入或死区内返回零向量
	static FVector GetWorldMoveInputDirection(const AActor* ContextActor, const FVector2D& MoveInput, float DeadZone);

	// 默认连招树，蓝图调用 HandleComboInput 时可不传 Tree
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combo", meta = (ToolTip = "默认连招树，蓝图调用 HandleComboInput 时可不传 Tree"))
	TObjectPtr<UKnsComboTreeData> DefaultComboTree;

	// 窗口未开启时缓存输入的最长秒数，用于预输入
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combo|Input", meta = (ClampMin = "0.0", ToolTip = "窗口未开启时缓存输入的最长秒数，用于预输入"))
	float InputBufferTime = 0.15f;

	// 切换到新连招节点时触发
	UPROPERTY(BlueprintAssignable, Category = "Combo|Events", meta = (ToolTip = "切换到新连招节点时触发"))
	FKnsComboNodeStarted OnComboNodeStarted;

	// 连招自然结束或被打断时触发
	UPROPERTY(BlueprintAssignable, Category = "Combo|Events", meta = (ToolTip = "连招自然结束或被打断时触发"))
	FKnsComboEnded OnComboEnded;

	UPROPERTY(BlueprintAssignable, Category = "Combo|Events", meta = (ToolTip = "切换到新招式时触发"))
	FKnsMoveStarted OnMoveStarted;

	UPROPERTY(BlueprintAssignable, Category = "Combo|Events", meta = (ToolTip = "连招激活状态变化时触发"))
	FKnsComboActiveChanged OnComboActiveChanged;

	UPROPERTY(BlueprintAssignable, Category = "Combo|Events", meta = (ToolTip = "韧性等级变化时触发"))
	FKnsPoiseChanged OnPoiseChanged;

	// 统一输入入口：空闲时起手，窗口开启时接段，窗口未开时缓存输入
	UFUNCTION(BlueprintCallable, Category = "Combo", meta = (ToolTip = "统一输入入口：空闲时起手，窗口开启时接段，窗口未开时缓存输入"))
	bool HandleComboInput(UKnsComboTreeData* Tree, FGameplayTag InputTag);

	// 只尝试从 RootLinks 起手连招，成功返回 true
	UFUNCTION(BlueprintCallable, Category = "Combo", meta = (ToolTip = "只尝试从 RootLinks 起手连招，成功返回 true"))
	bool TryStartCombo(UKnsComboTreeData* Tree, FGameplayTag InputTag);

	// 只尝试从当前节点 Links 接下一段，需要输入窗口开启
	UFUNCTION(BlueprintCallable, Category = "Combo", meta = (ToolTip = "只尝试从当前节点 Links 接下一段，需要输入窗口开启"))
	bool TryAdvanceCombo(FGameplayTag InputTag);

	// 由 ComboWindow NotifyState 调用，打开 / 关闭输入窗口
	UFUNCTION(BlueprintCallable, Category = "Combo", meta = (ToolTip = "由 ComboWindow NotifyState 调用，打开 / 关闭输入窗口"))
	void SetComboWindowOpen(bool bOpen);

	// 主动取消当前连招，可同时停止蒙太奇
	UFUNCTION(BlueprintCallable, Category = "Combo", meta = (ToolTip = "主动取消当前连招，可同时停止蒙太奇"))
	void CancelCombo(bool bStopMontage = true);

	// 当前是否正在连招中
	UFUNCTION(BlueprintPure, Category = "Combo", meta = (ToolTip = "当前是否正在连招中"))
	bool IsComboActive() const;

	// 取出当前节点数据，成功返回 true
	UFUNCTION(BlueprintCallable, Category = "Combo", meta = (ToolTip = "取出当前节点数据，成功返回 true"))
	bool GetCurrentNode(FComboNode& OutNode) const;

	// 当前正在使用的连招树
	UFUNCTION(BlueprintPure, Category = "Combo", meta = (ToolTip = "当前正在使用的连招树"))
	UKnsComboTreeData* GetActiveTree() const;

	// 当前节点 ID
	UFUNCTION(BlueprintPure, Category = "Combo", meta = (ToolTip = "当前节点 ID"))
	FName GetCurrentNodeId() const;

	// 当前正在执行的招式数据
	UFUNCTION(BlueprintPure, Category = "Combo", meta = (ToolTip = "当前正在执行的招式数据"))
	UKnsMoveDefinition* GetCurrentMoveDefinition() const;

	// 当前招式的原始韧性等级
	UFUNCTION(BlueprintPure, Category = "Combo|Poise", meta = (ToolTip = "当前招式的原始韧性等级"))
	int32 GetBaseMovePoiseLevel() const;

	// 当前实际参与受击打断判定的韧性等级，霸体时返回 99
	UFUNCTION(BlueprintPure, Category = "Combo|Poise", meta = (ToolTip = "当前实际参与受击打断判定的韧性等级，霸体时返回 99"))
	int32 GetEffectivePoiseLevel() const;

	// 由 SuperArmor NotifyState 调用，临时把有效韧性置为 99
	UFUNCTION(BlueprintCallable, Category = "Combo|Poise", meta = (ToolTip = "由 SuperArmor NotifyState 调用，临时把有效韧性置为 99"))
	void SetSuperArmor(bool bActive);

	// 命中条件成立时调用，例如碰到敌人挂 Combo.Condition.Touch 并检查自动派生
	UFUNCTION(BlueprintCallable, Category = "Combo|Condition", meta = (ToolTip = "命中条件成立时调用，例如碰到敌人挂 Combo.Condition.Touch 并检查自动派生"))
	void AddHitConditionTag(FGameplayTag Tag);

	// 根据传入的移动输入 Vector2D 实时计算相对角色面朝的 Input.Move.* 标签
	UFUNCTION(BlueprintPure, Category = "Combo|Input", meta = (ToolTip = "根据传入的移动输入 Vector2D 实时计算相对角色面朝的 Input.Move.* 标签"))
	FGameplayTag GetMoveInputDirectionTag(FVector2D MoveInput, float DeadZone = 0.25f, bool bDetectEightDirections = false) const;

protected:
	bool PlayComboMontage(const FComboNode& Node);
	bool LinkMatches(const FComboLink& Link, const FGameplayTag& InputTag) const;
	const FComboLink* FindBestLink(const TArray<FComboLink>& Links, const FGameplayTag& InputTag) const;
	bool TryAutoAdvanceCombo();
	void ConsumeBufferedInput();
	void ClearConditionTags();
	void EndCombo(bool bCancelled);
	UAbilitySystemComponent* GetAbilitySystemComponent() const;
	USkeletalMeshComponent* GetOwnerMesh() const;

	UFUNCTION()
	void HandleMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	UFUNCTION()
	void HandleCancelRequested();

	UFUNCTION()
	void HandleHitConditionTagRequested(FGameplayTag Tag);

	void LogCombatEvent(const FString& EventName, const FColor& Color, const FString& Payload);

	UPROPERTY(Transient)
	TObjectPtr<UKnsComboTreeData> ActiveTree;

	UPROPERTY(Transient)
	FName CurrentNodeId;

	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> CurrentMontage;

	UPROPERTY(Transient)
	TObjectPtr<UKnsMoveDefinition> ActiveMoveDefinition;

	UPROPERTY(Transient)
	int32 BaseMovePoiseLevel = 0;

	UPROPERTY(Transient)
	int32 EffectivePoiseLevel = 0;

	UPROPERTY(Transient)
	bool bSuperArmorActive = false;

	UPROPERTY(Transient)
	bool bAttackingTagAdded = false;

	UPROPERTY(Transient)
	TObjectPtr<UKnsCombatContextComponent> BoundCombatContext;

	UPROPERTY(Transient)
	FGameplayTagContainer ActiveConditionTags;

	UPROPERTY(Transient)
	bool bComboWindowOpen = false;

	UPROPERTY(Transient)
	FGameplayTag PendingInputTag;

	UPROPERTY(Transient)
	float PendingInputTime = 0.f;

	UPROPERTY(Transient)
	FGameplayTagContainer ActiveNodeGrantedTags;
};
