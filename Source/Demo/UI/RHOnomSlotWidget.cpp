#include "RHOnomSlotWidget.h"

#include "Components/Image.h"

void URHOnomSlotWidget::SetSlotState(ERHOnomValue State)
{
	if (!SlotIcon)
	{
		return;
	}

	FLinearColor Color = EmptyColor;
	if (State == ERHOnomValue::Positive)
	{
		Color = PositiveColor;
	}
	else if (State == ERHOnomValue::Negative)
	{
		Color = NegativeColor;
	}
	else if (State == ERHOnomValue::Broken)
	{
		Color = BrokenColor;
	}
	else if (State == ERHOnomValue::Neutral)
	{
		Color = NeutralColor;
	}

	SlotIcon->SetColorAndOpacity(Color);
}
