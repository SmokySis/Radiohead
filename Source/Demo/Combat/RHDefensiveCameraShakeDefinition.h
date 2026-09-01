#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraShakeBase.h"
#include "Engine/DataAsset.h"
#include "RHDefensiveCameraShakeDefinition.generated.h"

UENUM(BlueprintType)
enum class ERHDefensiveShakeType : uint8
{
	GuardHit UMETA(DisplayName = "Guard Hit"),
	BigBreak UMETA(DisplayName = "Big Break"),
	Parry UMETA(DisplayName = "Parry")
};

USTRUCT(BlueprintType)
struct FRHDefensiveShakeEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CameraShake")
	ERHDefensiveShakeType Type = ERHDefensiveShakeType::GuardHit;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CameraShake")
	TSubclassOf<UCameraShakeBase> CameraShake;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CameraShake", meta = (ClampMin = "0", ClampMax = "10"))
	float ShakeScale = 1.f;
};

UCLASS(BlueprintType)
class DEMO_API URHDefensiveCameraShakeDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CameraShake")
	TArray<FRHDefensiveShakeEntry> Entries;

	UFUNCTION(BlueprintPure, Category = "CameraShake")
	bool GetShake(ERHDefensiveShakeType Type, FRHDefensiveShakeEntry& OutEntry) const;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
};
