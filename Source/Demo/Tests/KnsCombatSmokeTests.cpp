#include "Misc/AutomationTest.h"

#include "Demo/Combat/KnsMoveDefinition.h"
#include "Demo/Combo/KnsComboTreeData.h"
#include "Demo/GAS/KnsDamageExecutionCalculation.h"
#include "UObject/UObjectGlobals.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FKnsMoveDefinition_ValidateMissingMontage,
	"Project.Unit.KnsMoveDefinition.MissingMontage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::SmokeFilter)

bool FKnsMoveDefinition_ValidateMissingMontage::RunTest(const FString& Parameters)
{
	UKnsMoveDefinition* Move = NewObject<UKnsMoveDefinition>();
	TestNotNull("MoveDefinition should be created", Move);

	TArray<FText> Errors;
	Move->ValidateMove(Errors, TEXT("TestNode"), false);
	TestTrue("Missing montage should produce validation error", Errors.Num() > 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FKnsComboTree_ValidateMissingMove,
	"Project.Unit.KnsComboTree.MissingMove",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::SmokeFilter)

bool FKnsComboTree_ValidateMissingMove::RunTest(const FString& Parameters)
{
	UKnsComboTreeData* Tree = NewObject<UKnsComboTreeData>();
	TestNotNull("ComboTree should be created", Tree);

	FComboNode Node;
	Node.NodeId = TEXT("A");
	Tree->Nodes.Add(Node);

	TArray<FText> Errors;
	Tree->ValidateTree(Errors);
	TestTrue("Missing MoveDefinition should produce validation error", Errors.Num() > 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FKnsDamageCalculation_Base,
	"Project.Unit.KnsDamageCalculation.Base",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::SmokeFilter)

bool FKnsDamageCalculation_Base::RunTest(const FString& Parameters)
{
	const float Damage = UKnsDamageExecutionCalculation::CalculateDamage(100.f, 0.f, 0.f, 0.f, 1.f, 1.5f, false);
	TestEqual("Base damage should be 100", Damage, 100.f, 0.01f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FKnsDamageCalculation_Reductions,
	"Project.Unit.KnsDamageCalculation.Reductions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::SmokeFilter)

bool FKnsDamageCalculation_Reductions::RunTest(const FString& Parameters)
{
	const float Damage = UKnsDamageExecutionCalculation::CalculateDamage(100.f, 0.5f, 100.f, 0.2f, 1.f, 1.5f, false);
	TestEqual("Damage after reductions should be 20", Damage, 20.f, 0.01f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FKnsDamageCalculation_Crit,
	"Project.Unit.KnsDamageCalculation.Crit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::SmokeFilter)

bool FKnsDamageCalculation_Crit::RunTest(const FString& Parameters)
{
	const float Damage = UKnsDamageExecutionCalculation::CalculateDamage(100.f, 0.f, 0.f, 0.f, 1.f, 1.5f, true);
	TestEqual("Critical damage should be 150", Damage, 150.f, 0.01f);
	return true;
}
