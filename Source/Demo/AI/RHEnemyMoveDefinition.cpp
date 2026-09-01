#include "RHEnemyMoveDefinition.h"

FPrimaryAssetId URHEnemyMoveDefinition::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("RHEnemyMove"), MoveId);
}
