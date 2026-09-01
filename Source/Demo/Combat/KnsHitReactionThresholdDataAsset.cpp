#include "KnsHitReactionThresholdDataAsset.h"

EKnsHitReactionStrength UKnsHitReactionThresholdDataAsset::GetStrengthForPoiseDiff(int32 Diff) const
{
	if (Diff >= KnockdownPoiseDiff)
	{
		return EKnsHitReactionStrength::Knockdown;
	}
	if (Diff >= HeavyPoiseDiff)
	{
		return EKnsHitReactionStrength::Heavy;
	}
	if (Diff >= MediumPoiseDiff)
	{
		return EKnsHitReactionStrength::Medium;
	}
	return EKnsHitReactionStrength::Light;
}

FPrimaryAssetId UKnsHitReactionThresholdDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(FPrimaryAssetType(TEXT("KnsHitReactionThresholdDataAsset")), GetFName());
}
