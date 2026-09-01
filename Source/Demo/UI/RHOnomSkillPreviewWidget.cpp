#include "RHOnomSkillPreviewWidget.h"

#include "Demo/Onom/RHOnomActionDefinition.h"

void URHOnomSkillPreviewWidget::SetSkill(URHOnomActionDefinition* Skill, const FRHOnomConsumptionData& Available)
{
	if (!Skill)
	{
		ClearSkill();
		return;
	}

	const bool bAvailable = Skill->MatchesConsumption(Available);

	if (SkillNameText)
	{
		SkillNameText->SetText(Skill->DisplayName);
		SkillNameText->SetColorAndOpacity(bAvailable ? AvailableColor : UnavailableColor);
	}

	if (CostText)
	{
		CostText->SetText(FText::FromString(FString::Printf(TEXT("消耗 %d"), Skill->RequiredCount)));
	}

	if (AvailabilityText)
	{
		AvailabilityText->SetText(FText::FromString(bAvailable ? TEXT("OK") : TEXT("X")));
		AvailabilityText->SetColorAndOpacity(bAvailable ? AvailableColor : UnavailableColor);
	}

	if (AvailabilityIcon)
	{
		AvailabilityIcon->SetColorAndOpacity(bAvailable ? AvailableColor : UnavailableColor);
	}
}

void URHOnomSkillPreviewWidget::ClearSkill()
{
	if (SkillNameText)
	{
		SkillNameText->SetText(FText::GetEmpty());
	}
	if (CostText)
	{
		CostText->SetText(FText::GetEmpty());
	}
	if (AvailabilityText)
	{
		AvailabilityText->SetText(FText::GetEmpty());
	}
	if (AvailabilityIcon)
	{
		AvailabilityIcon->SetColorAndOpacity(UnavailableColor);
	}
}
