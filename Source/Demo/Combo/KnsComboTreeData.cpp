#include "KnsComboTreeData.h"

#include "Demo/Combat/KnsMoveDefinition.h"
#include "Animation/AnimMontage.h"

#if WITH_EDITOR
#include "UObject/UnrealType.h"
#endif

const FComboNode* UKnsComboTreeData::FindNode(FName NodeId) const
{
	return Nodes.FindByPredicate([NodeId](const FComboNode& Node)
	{
		return Node.NodeId == NodeId;
	});
}

void UKnsComboTreeData::ValidateTree(TArray<FText>& OutErrors) const
{
	TSet<FName> SeenNodeIds;

	for (const FComboNode& Node : Nodes)
	{
		if (Node.NodeId.IsNone())
		{
			OutErrors.Add(FText::FromString(TEXT("存在 NodeId 为空的节点。")));
		}
		else if (SeenNodeIds.Contains(Node.NodeId))
		{
			OutErrors.Add(FText::FromString(FString::Printf(TEXT("重复 NodeId: %s"), *Node.NodeId.ToString())));
		}
		else
		{
			SeenNodeIds.Add(Node.NodeId);
		}

		if (Node.Move.IsNull())
		{
			OutErrors.Add(FText::FromString(FString::Printf(TEXT("节点 %s 未配置 MoveDefinition。"), *Node.NodeId.ToString())));
		}
		else if (const UKnsMoveDefinition* Move = Node.Move.LoadSynchronous())
		{
			bool bRequiresComboWindow = false;
			for (const FComboLink& Link : Node.Links)
			{
				if (!Link.bAutoTrigger && Link.InputTag.IsValid())
				{
					bRequiresComboWindow = true;
					break;
				}
			}

			Move->ValidateMove(OutErrors, Node.NodeId, bRequiresComboWindow);
		}
	}

	auto ValidateLinks = [this, &SeenNodeIds, &OutErrors](const TArray<FComboLink>& Links, const FString& OwnerLabel)
	{
		for (const FComboLink& Link : Links)
		{
			if (Link.bAutoTrigger && Link.InputTag.IsValid())
			{
				OutErrors.Add(FText::FromString(FString::Printf(TEXT("%s 的自动派生转移不应配置 InputTag。"), *OwnerLabel)));
			}

			if (Link.bAutoTrigger && Link.RequiredTags.IsEmpty())
			{
				OutErrors.Add(FText::FromString(FString::Printf(TEXT("%s 的自动派生转移缺少 RequiredTags 条件。"), *OwnerLabel)));
			}

			if (!Link.bAutoTrigger && !Link.InputTag.IsValid())
			{
				OutErrors.Add(FText::FromString(FString::Printf(TEXT("%s 存在未配置 InputTag 的转移。"), *OwnerLabel)));
			}

			if (Link.TargetNodeId.IsNone())
			{
				OutErrors.Add(FText::FromString(FString::Printf(TEXT("%s 存在未配置目标节点的转移。"), *OwnerLabel)));
			}
			else if (!SeenNodeIds.Contains(Link.TargetNodeId))
			{
				OutErrors.Add(FText::FromString(FString::Printf(TEXT("%s 的转移指向不存在的节点: %s"), *OwnerLabel, *Link.TargetNodeId.ToString())));
			}
		}
	};

	ValidateLinks(RootLinks, TEXT("Root"));
	ValidateLinks(UniversalLinks, TEXT("Universal"));

	for (const FComboNode& Node : Nodes)
	{
		ValidateLinks(Node.Links, Node.NodeId.ToString());
	}

	// 节点可达性：从 Root/Universal 出发，沿 Links 能到达的节点才算可用
	TSet<FName> ReachableNodeIds;
	auto AddLinkTargets = [&ReachableNodeIds](const TArray<FComboLink>& Links)
	{
		for (const FComboLink& Link : Links)
		{
			if (!Link.TargetNodeId.IsNone())
			{
				ReachableNodeIds.Add(Link.TargetNodeId);
			}
		}
	};

	AddLinkTargets(RootLinks);
	AddLinkTargets(UniversalLinks);

	bool bChanged = true;
	while (bChanged)
	{
		bChanged = false;
		for (const FComboNode& Node : Nodes)
		{
			if (ReachableNodeIds.Contains(Node.NodeId))
			{
				for (const FComboLink& Link : Node.Links)
				{
					if (!Link.TargetNodeId.IsNone() && !ReachableNodeIds.Contains(Link.TargetNodeId))
					{
						ReachableNodeIds.Add(Link.TargetNodeId);
						bChanged = true;
					}
				}
			}
		}
	}

	for (const FComboNode& Node : Nodes)
	{
		if (!ReachableNodeIds.Contains(Node.NodeId))
		{
			OutErrors.Add(FText::FromString(FString::Printf(TEXT("节点 %s 从任何 Root/Universal 入口都不可达。"), *Node.NodeId.ToString())));
		}
	}
}

#if WITH_EDITOR
void UKnsComboTreeData::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	TArray<FText> Errors;
	ValidateTree(Errors);

	for (const FText& Error : Errors)
	{
		UE_LOG(LogTemp, Warning, TEXT("ComboTree %s: %s"), *GetName(), *Error.ToString());
	}
}
#endif

FPrimaryAssetId UKnsComboTreeData::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(FPrimaryAssetType(TEXT("KnsComboTreeData")), GetFName());
}
