#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "KnsComboTypes.h"
#include "KnsComboTreeData.generated.h"

UCLASS(BlueprintType)
class DEMO_API UKnsComboTreeData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// 资产显示名，方便在编辑器资产浏览器里辨认
	UPROPERTY(EditAnywhere, BlueprintReadOnly, AssetRegistrySearchable, Category = "Combo", meta = (ToolTip = "资产显示名，方便在编辑器资产浏览器里辨认"))
	FText DisplayName;

	// 这套连招所属的武器 Tag，例如 Weapon.Nodachi
	UPROPERTY(EditAnywhere, BlueprintReadOnly, AssetRegistrySearchable, Category = "Combo", meta = (ToolTip = "这套连招所属的武器 Tag，例如 Weapon.Nodachi"))
	FGameplayTag WeaponTag;

	// 从空闲状态起手的入口转移，按输入 / Tag 条件 / 优先级匹配
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combo", meta = (ToolTip = "从空闲状态起手的入口转移，按输入 / Tag 条件 / 优先级匹配"))
	TArray<FComboLink> RootLinks;

	// 通用派生：任意节点都会检查这些转移，空闲起手时也会检查；目标节点可被任意招式派生，也可作为起手激活
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combo", meta = (ToolTip = "通用派生：任意节点都会检查这些转移，空闲起手时也会检查；目标节点可被任意招式派生，也可作为起手激活"))
	TArray<FComboLink> UniversalLinks;

	// 连招树的所有节点，NodeId 必须唯一
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combo", meta = (ToolTip = "连招树的所有节点，NodeId 必须唯一"))
	TArray<FComboNode> Nodes;

	// 按 NodeId 查找节点；找不到返回 nullptr
	const FComboNode* FindNode(FName NodeId) const;

	// 校验整棵树：重复 NodeId、悬空转移、空蒙太奇、无效 InputTag
	void ValidateTree(TArray<FText>& OutErrors) const;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
};
