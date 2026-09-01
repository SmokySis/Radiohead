#include "KnsMoveDefinition.h"

#include "Animation/AnimMontage.h"
#include "Animation/AnimTypes.h"
#include "Demo/Combat/KnsHitboxNotifyState.h"
#include "Demo/Combo/KnsComboWindowNotifyState.h"

#if WITH_EDITOR
#include "UObject/UnrealType.h"
#endif

void UKnsMoveDefinition::ValidateMove(TArray<FText>& OutErrors, const FName& NodeId, bool bRequiresComboWindow) const
{
	if (Montage.IsNull())
	{
		OutErrors.Add(FText::FromString(FString::Printf(TEXT("节点 %s 的招式 %s 未配置 Montage。"), *NodeId.ToString(), *GetName())));
		return;
	}

	UAnimMontage* LoadedMontage = Montage.LoadSynchronous();
	if (!LoadedMontage)
	{
		OutErrors.Add(FText::FromString(FString::Printf(TEXT("节点 %s 的蒙太奇加载失败。"), *NodeId.ToString())));
		return;
	}

	if (!SectionName.IsNone() && LoadedMontage->GetSectionIndex(SectionName) == INDEX_NONE)
	{
		OutErrors.Add(FText::FromString(FString::Printf(TEXT("节点 %s 的 Section %s 不存在。"), *NodeId.ToString(), *SectionName.ToString())));
	}

	bool bHasComboWindow = false;
	bool bHasHitbox = false;

	for (const FAnimNotifyEvent& NotifyEvent : LoadedMontage->Notifies)
	{
		if (NotifyEvent.NotifyStateClass && NotifyEvent.NotifyStateClass->IsA(UKnsComboWindowNotifyState::StaticClass()))
		{
			bHasComboWindow = true;
		}
		if (NotifyEvent.NotifyStateClass && NotifyEvent.NotifyStateClass->IsA(UKnsHitboxNotifyState::StaticClass()))
		{
			bHasHitbox = true;
		}
	}

	if (bRequiresComboWindow && !bHasComboWindow)
	{
		OutErrors.Add(FText::FromString(FString::Printf(TEXT("节点 %s 存在输入派生，但蒙太奇缺少 Kns Combo Window。"), *NodeId.ToString())));
	}

	if (HitStopLevel > 0 && !bHasHitbox)
	{
		OutErrors.Add(FText::FromString(FString::Printf(TEXT("节点 %s 配置了卡肉等级，但蒙太奇缺少 Kns Hitbox。"), *NodeId.ToString())));
	}
}

#if WITH_EDITOR
void UKnsMoveDefinition::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	TArray<FText> Errors;
	ValidateMove(Errors, GetFName(), false);

	for (const FText& Error : Errors)
	{
		UE_LOG(LogTemp, Warning, TEXT("MoveDefinition %s: %s"), *GetName(), *Error.ToString());
	}
}
#endif

FPrimaryAssetId UKnsMoveDefinition::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(FPrimaryAssetType(TEXT("KnsMoveDefinition")), GetFName());
}
