#include "RHDefensiveCameraShakeDefinition.h"

bool URHDefensiveCameraShakeDefinition::GetShake(ERHDefensiveShakeType Type, FRHDefensiveShakeEntry& OutEntry) const
{
	for (const FRHDefensiveShakeEntry& Entry : Entries)
	{
		if (Entry.Type == Type)
		{
			OutEntry = Entry;
			return true;
		}
	}
	return false;
}

FPrimaryAssetId URHDefensiveCameraShakeDefinition::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(FPrimaryAssetType(TEXT("RHDefensiveCameraShake")), GetFName());
}
