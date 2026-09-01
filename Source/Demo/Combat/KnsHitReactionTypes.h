#pragma once

#include "CoreMinimal.h"
#include "KnsHitReactionTypes.generated.h"

UENUM(BlueprintType)
enum class EKnsHitDirection : uint8
{
	Front UMETA(DisplayName = "Front"),
	Back UMETA(DisplayName = "Back"),
	Left UMETA(DisplayName = "Left"),
	Right UMETA(DisplayName = "Right")
};

UENUM(BlueprintType)
enum class EKnsHitReactionStrength : uint8
{
	Light UMETA(DisplayName = "Light"),
	Medium UMETA(DisplayName = "Medium"),
	Heavy UMETA(DisplayName = "Heavy"),
	Knockdown UMETA(DisplayName = "Knockdown")
};
