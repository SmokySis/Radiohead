#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "KnsCombatDebugSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogKnsCombat, Log, All);

USTRUCT(BlueprintType)
struct FKnsCombatDebugEvent
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Debug")
	FString EventName;

	UPROPERTY(BlueprintReadOnly, Category = "Debug")
	FString Payload;

	UPROPERTY(BlueprintReadOnly, Category = "Debug")
	FColor Color = FColor::White;

	UPROPERTY(BlueprintReadOnly, Category = "Debug")
	double Time = 0.0;
};

UCLASS()
class DEMO_API UKnsCombatDebugSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	void LogEvent(const FString& EventName, const FColor& Color, const FString& Payload);

	UFUNCTION(BlueprintCallable, Category = "Kns|Debug", meta = (ToolTip = "清空调试事件历史"))
	void ClearLog();

	const TArray<FKnsCombatDebugEvent>& GetEvents() const;

protected:
	UPROPERTY(Transient)
	TArray<FKnsCombatDebugEvent> Events;

};
