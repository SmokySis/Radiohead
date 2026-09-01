#include "KnsHitReactionSettingsDataAsset.h"

#include "Animation/AnimMontage.h"

UAnimMontage* UKnsHitReactionSettingsDataAsset::GetReactionMontage(EKnsHitReactionStrength Strength, EKnsHitDirection Direction) const
{
	for (const FHitReactionTypeRow& Row : Reactions)
	{
		if (Row.Strength == Strength)
		{
			return Row.Montage.LoadSynchronous();
		}
	}
	return nullptr;
}

UAnimMontage* UKnsHitReactionSettingsDataAsset::GetDefensiveHitMontage() const
{
	return DefensiveHitMontage.LoadSynchronous();
}

UAnimMontage* UKnsHitReactionSettingsDataAsset::GetDefensiveBreakMontage() const
{
	return DefensiveBreakMontage.LoadSynchronous();
}

FName UKnsHitReactionSettingsDataAsset::GetSectionNameForDirection(EKnsHitDirection Direction)
{
	switch (Direction)
	{
	case EKnsHitDirection::Front:
		return TEXT("F");
	case EKnsHitDirection::Back:
		return TEXT("B");
	case EKnsHitDirection::Left:
		return TEXT("L");
	case EKnsHitDirection::Right:
		return TEXT("R");
	default:
		return TEXT("F");
	}
}

FString UKnsHitReactionSettingsDataAsset::GetDebugSummary() const
{
	FString Summary;
	for (const FHitReactionTypeRow& Row : Reactions)
	{
		Summary += FString::Printf(
			TEXT("[%d]=%s; "),
			(int32)Row.Strength,
			Row.Montage.IsNull() ? TEXT("NULL") : *Row.Montage.GetAssetName());
	}
	return Summary;
}

FPrimaryAssetId UKnsHitReactionSettingsDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(FPrimaryAssetType(TEXT("KnsHitReactionSettingsDataAsset")), GetFName());
}
