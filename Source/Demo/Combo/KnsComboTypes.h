#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "KnsComboTypes.generated.h"

class UAnimMontage;
class UKnsMoveDefinition;

USTRUCT(BlueprintType)
struct FComboLink
{
	GENERATED_BODY()

	// 目标节点 ID，必须与 Nodes 数组中的 NodeId 一致
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo", meta = (ToolTip = "目标节点 ID，必须与 Nodes 数组中的 NodeId 一致"))
	FName TargetNodeId;

	// 触发这条转移需要的输入标签，例如 Input.YB / Input.LTYB
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo", meta = (ToolTip = "触发这条转移需要的输入标签，例如 Input.YB / Input.LTYB"))
	FGameplayTag InputTag;

	// 角色身上必须同时拥有这些 Tag，转移才可用
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo", meta = (ToolTip = "角色身上必须同时拥有这些 Tag，转移才可用"))
	FGameplayTagContainer RequiredTags;

	// 角色身上只要拥有其中任意一个 Tag，转移就不可用
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo", meta = (ToolTip = "角色身上只要拥有其中任意一个 Tag，转移就不可用"))
	FGameplayTagContainer BlockedTags;

	// 多条转移同时满足时，数字越小越优先，0 最高
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo", meta = (ToolTip = "多条转移同时满足时，数字越小越优先，0 最高"))
	int32 Priority = 0;

	// 自动派生：不需要输入，条件（RequiredTags）满足时自动进入目标节点
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo", meta = (ToolTip = "自动派生：不需要输入，条件（RequiredTags）满足时自动进入目标节点"))
	bool bAutoTrigger = false;
};

USTRUCT(BlueprintType)
struct FComboNode
{
	GENERATED_BODY()

	// 节点唯一标识，用于 RootLinks / Links 指向
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo", meta = (ToolTip = "节点唯一标识，用于 RootLinks / Links 指向"))
	FName NodeId;

	// 该节点实际执行的招式数据
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo", meta = (ToolTip = "该节点实际执行的招式数据"))
	TSoftObjectPtr<UKnsMoveDefinition> Move;

	// 从当前节点可进入的下一段招式
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo", meta = (ToolTip = "从当前节点可进入的下一段招式"))
	TArray<FComboLink> Links;
};
