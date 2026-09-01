#include "KnsHitStopSettingsDataAsset.h"

bool UKnsHitStopSettingsDataAsset::GetSettings(int32 Level, FHitStopLevelSettings& OutSettings) const
{
	for (const FHitStopLevelSettings& Settings : Levels)
	{
		if (Settings.Level == Level)
		{
			OutSettings = Settings;
			return true;
		}
	}

	return false;
}

FPrimaryAssetId UKnsHitStopSettingsDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(FPrimaryAssetType(TEXT("KnsHitStopSettingsDataAsset")), GetFName());
}
