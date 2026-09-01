#include "RHChargeGaugeWidget.h"

#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

void URHChargeGaugeWidget::SetChargePercent(float Percent)
{
	if (ChargeBar)
	{
		ChargeBar->SetPercent(FMath::Clamp(Percent / 100.f, 0.f, 1.f));
	}

	if (ChargeText)
	{
		ChargeText->SetText(FText::FromString(FString::Printf(TEXT("%.0f%%"), FMath::Clamp(Percent, 0.f, 100.f))));
	}
}
