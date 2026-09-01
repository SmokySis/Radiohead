#include "RHCoreDefinition.h"

#include "Internationalization/Text.h"

URHCoreDefinition::URHCoreDefinition()
{
	DisplayName = NSLOCTEXT("RadioHead", "DamagedCore", "Damaged Core");
	DamageTakenRule.bEnabled = true;
	DamageTakenRule.Mode = ERHOnomRuleMode::Remove;
	DamageTakenRule.Type = ERHOnomValue::Positive;
	DamageTakenRule.Count = 1;
}
