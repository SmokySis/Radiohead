#include "RHOnomSettings.h"

URHOnomSettings::URHOnomSettings()
{
	ResonanceDecaySeconds.Add(10.f);
	ResonanceDecaySeconds.Add(8.f);
	ResonanceDecaySeconds.Add(6.f);

	ResonanceLevelFactors.Add(1.1f);
	ResonanceLevelFactors.Add(1.5f);
	ResonanceLevelFactors.Add(1.8f);
}

float URHOnomSettings::GetResonanceDecayForLayer(int32 Layer) const
{
	if (Layer >= 1 && ResonanceDecaySeconds.IsValidIndex(Layer - 1))
	{
		return ResonanceDecaySeconds[Layer - 1];
	}
	return 8.f;
}

float URHOnomSettings::GetResonanceLevelFactor(int32 Level) const
{
	if (Level >= 1 && ResonanceLevelFactors.IsValidIndex(Level - 1))
	{
		return ResonanceLevelFactors[Level - 1];
	}
	return 1.f;
}
