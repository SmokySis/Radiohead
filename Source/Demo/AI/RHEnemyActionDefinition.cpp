#include "RHEnemyActionDefinition.h"

FPrimaryAssetId URHEnemyActionDefinition::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("RHEnemyAction"), ActionId);
}
