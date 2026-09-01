#include "RHEnemyMovePoolDefinition.h"

FPrimaryAssetId URHEnemyMovePoolDefinition::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("RHEnemyMovePool"), GetFName());
}
