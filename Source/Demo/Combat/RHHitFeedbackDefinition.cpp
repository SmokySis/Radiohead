#include "RHHitFeedbackDefinition.h"

bool URHHitFeedbackDefinition::GetEntry(const TArray<FRHHitFeedbackPolarityEntry>& Entries, ERHOnomPolarity Polarity, FRHHitFeedbackPolarityEntry& OutEntry) const
{
	for (const FRHHitFeedbackPolarityEntry& Entry : Entries)
	{
		if (Entry.Polarity == Polarity)
		{
			OutEntry = Entry;
			return true;
		}
	}
	return false;
}
