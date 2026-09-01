#include "KnsCombatDebugSubsystem.h"

#include "VisualLogger/VisualLogger.h"

DEFINE_LOG_CATEGORY(LogKnsCombat);

void UKnsCombatDebugSubsystem::LogEvent(const FString& EventName, const FColor& Color, const FString& Payload)
{
	FKnsCombatDebugEvent Event;
	Event.EventName = EventName;
	Event.Payload = Payload;
	Event.Color = Color;
	Event.Time = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;

	Events.Add(Event);
	if (Events.Num() > 256)
	{
		Events.RemoveAt(0, Events.Num() - 256);
	}

	UE_VLOG(GetWorld(), LogKnsCombat, Log, TEXT("%s %s"), *EventName, *Payload);
}

void UKnsCombatDebugSubsystem::ClearLog()
{
	Events.Reset();
}

const TArray<FKnsCombatDebugEvent>& UKnsCombatDebugSubsystem::GetEvents() const
{
	return Events;
}
