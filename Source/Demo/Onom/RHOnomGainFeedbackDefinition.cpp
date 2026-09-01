#include "RHOnomGainFeedbackDefinition.h"

bool URHOnomGainFeedbackDefinition::GetEntry(ERHOnomPolarity Polarity, FRHOnomGainFeedbackEntry& OutEntry) const
{
	for (const FRHOnomGainFeedbackEntry& Entry : Entries)
	{
		if (Entry.Polarity == Polarity)
		{
			OutEntry = Entry;
			return true;
		}
	}
	return false;
}

FPrimaryAssetId URHOnomGainFeedbackDefinition::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(FPrimaryAssetType(TEXT("RHOnomGainFeedbackDefinition")), GetFName());
}
