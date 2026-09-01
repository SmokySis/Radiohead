#include "RHOnomActionDefinition.h"

bool URHOnomActionDefinition::MatchesConsumption(const FRHOnomConsumptionData& Data) const
{
	return RHOnomMatchesRequirement(RequiredCount, RequirementPolarity, RequirementMode, Data);
}

bool URHOnomActionDefinition::ResolveActionData(const FRHOnomConsumptionData& Data, FRHOnomResolvedAction& OutData) const
{
	OutData = FRHOnomResolvedAction();

	ERHOnomPolarity Polarity = ERHOnomPolarity::None;
	if (Data.SignedSum > 0)
	{
		Polarity = ERHOnomPolarity::Major;
	}
	else if (Data.SignedSum < 0)
	{
		Polarity = ERHOnomPolarity::Minor;
	}
	else if (Data.SignedSum == 0 && Data.ConsumedCount > 0)
	{
		Polarity = ERHOnomPolarity::Neutral;
	}

	const int32 BaseHitPoise = HitPoise > 0 ? HitPoise : PoiseLevel;
	const FRHOnomActionVariant* Matched = nullptr;
	for (const FRHOnomActionVariant& Variant : Variants)
	{
		if (Variant.Polarity == Polarity)
		{
			Matched = &Variant;
			break;
		}
	}

	if (Matched)
	{
		OutData.Montage = Matched->Montage;
		OutData.SectionName = Matched->SectionName;
		OutData.PlayRate = Matched->PlayRate;
		OutData.Damage = Matched->Damage > 0 ? Matched->Damage : Damage;
		OutData.ResonanceDamage = Matched->ResonanceDamage > 0 ? Matched->ResonanceDamage : ResonanceDamage;
		OutData.CounterBarDamage = Matched->bOverrideCounterBarDamage ? Matched->CounterBarDamage : CounterBarDamage;
		OutData.HitPoise = Matched->HitPoise > 0 ? Matched->HitPoise : BaseHitPoise;
	}
	else
	{
		OutData.Montage = Montage;
		OutData.SectionName = SectionName;
		OutData.PlayRate = PlayRate;
		OutData.Damage = Damage;
		OutData.ResonanceDamage = ResonanceDamage;
		OutData.CounterBarDamage = CounterBarDamage;
		OutData.HitPoise = BaseHitPoise;
	}

	OutData.AttackTag = AttackTag;
	OutData.GrantedTags = GrantedTags;
	OutData.HitStopLevel = HitStopLevel;
	OutData.PoiseLevel = PoiseLevel;
	OutData.Resistance = Resistance;

	return !OutData.Montage.IsNull();
}

float URHOnomActionDefinition::GetDiscountMultiplier(const FRHOnomConsumptionData& Data) const
{
	if (RequirementMode == ERHOnomRequirementMode::UnrestrictedWithDiscount
		&& RequirementPolarity != ERHOnomPolarity::None
		&& RHGetConsumptionPolarity(Data) != RequirementPolarity)
	{
		return DiscountFactor;
	}
	return 1.f;
}
