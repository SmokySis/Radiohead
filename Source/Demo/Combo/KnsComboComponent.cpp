#include "KnsComboComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Demo/Combat/KnsCombatContextComponent.h"
#include "Demo/Combat/KnsMoveDefinition.h"
#include "Demo/Debug/KnsCombatDebugSubsystem.h"
#include "GameFramework/Character.h"
#include "KnsComboTreeData.h"
#include "Math/RotationMatrix.h"

UKnsComboComponent::UKnsComboComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UKnsComboComponent::BeginPlay()
{
	Super::BeginPlay();

	if (USkeletalMeshComponent* Mesh = GetOwnerMesh())
	{
		if (UAnimInstance* AnimInstance = Mesh->GetAnimInstance())
		{
			AnimInstance->OnMontageEnded.AddDynamic(this, &UKnsComboComponent::HandleMontageEnded);
		}
	}

	if (AActor* Owner = GetOwner())
	{
		if (UKnsCombatContextComponent* Context = Owner->FindComponentByClass<UKnsCombatContextComponent>())
		{
			BoundCombatContext = Context;
			Context->OnCancelRequested.AddDynamic(this, &UKnsComboComponent::HandleCancelRequested);
			Context->OnHitConditionTagRequested.AddDynamic(this, &UKnsComboComponent::HandleHitConditionTagRequested);
		}
	}
}

void UKnsComboComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (PendingInputTag.IsValid() && GetWorld())
	{
		if (GetWorld()->GetTimeSeconds() - PendingInputTime > InputBufferTime)
		{
			PendingInputTag = FGameplayTag::EmptyTag;
		}
	}
}

bool UKnsComboComponent::HandleComboInput(UKnsComboTreeData* Tree, FGameplayTag InputTag)
{
	if (IsComboActive())
	{
		// 连招中：窗口开着直接接段，窗口没开先缓存，等窗口打开再消费
		if (bComboWindowOpen)
		{
			return TryAdvanceCombo(InputTag);
		}

		PendingInputTag = InputTag;
		PendingInputTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
		LogCombatEvent(TEXT("InputBuffered"), FColor::Yellow, InputTag.ToString());
		return false;
	}

	return TryStartCombo(Tree, InputTag);
}

FVector UKnsComboComponent::GetWorldMoveInputDirection(const AActor* ContextActor, const FVector2D& MoveInput, float DeadZone)
{
	if (!ContextActor || DeadZone <= 0.f || MoveInput.SizeSquared() < FMath::Square(DeadZone))
	{
		return FVector::ZeroVector;
	}

	FVector WorldMove = FVector::ZeroVector;
	if (const APawn* Pawn = Cast<APawn>(ContextActor))
	{
		if (const AController* Controller = Pawn->GetController())
		{
			const FRotator ControlRotation = Controller->GetControlRotation();
			const FVector ControlForward = ControlRotation.Vector();
			const FVector ControlRight = FRotationMatrix(ControlRotation).GetUnitAxis(EAxis::Y);
			WorldMove = (ControlForward * MoveInput.Y) + (ControlRight * MoveInput.X);
		}
	}

	return WorldMove.GetSafeNormal2D();
}

FGameplayTag UKnsComboComponent::GetMoveInputDirectionTag(FVector2D MoveInput, float DeadZone, bool bDetectEightDirections) const
{
	const FGameplayTag NoneTag = FGameplayTag::RequestGameplayTag(TEXT("Input.Move.None"), false);

	if (DeadZone <= 0.f)
	{
		return NoneTag;
	}

	// 摇杆幅度小于死区时视为没有移动输入
	if (MoveInput.SizeSquared() < FMath::Square(DeadZone))
	{
		return NoneTag;
	}

	// 把摇杆 XY 从镜头空间换算成世界方向
	FVector WorldMove = FVector::ZeroVector;
	if (const APawn* Pawn = Cast<APawn>(GetOwner()))
	{
		if (const AController* Controller = Pawn->GetController())
		{
			const FRotator ControlRotation = Controller->GetControlRotation();
			const FVector ControlForward = ControlRotation.Vector();
			const FVector ControlRight = FRotationMatrix(ControlRotation).GetUnitAxis(EAxis::Y);
			WorldMove = (ControlForward * MoveInput.Y) + (ControlRight * MoveInput.X);
		}
	}

	WorldMove = WorldMove.GetSafeNormal2D();
	if (WorldMove.IsNearlyZero())
	{
		return NoneTag;
	}

	const AActor* Owner = GetOwner();
	if (!Owner)
	{
		return NoneTag;
	}

	const FVector ActorForward = Owner->GetActorForwardVector().GetSafeNormal2D();
	const FVector ActorRight = Owner->GetActorRightVector().GetSafeNormal2D();

	// 用点积得到相对角色面朝的方向角：0 为前方，正角度偏右，负角度偏左
	const float ForwardDot = FVector::DotProduct(WorldMove, ActorForward);
	const float RightDot = FVector::DotProduct(WorldMove, ActorRight);
	const float AngleDeg = FMath::RadiansToDegrees(FMath::Atan2(RightDot, ForwardDot));

	auto MakeMoveTag = [](const TCHAR* DirectionSuffix) -> FGameplayTag
	{
		const FName TagName = FName(FString::Printf(TEXT("Input.Move.%s"), DirectionSuffix));
		return FGameplayTag::RequestGameplayTag(TagName, false);
	};

	if (bDetectEightDirections)
	{
		static const TCHAR* DirectionNames8[] = {
			TEXT("Forward"), TEXT("ForwardRight"), TEXT("Right"), TEXT("BackwardRight"),
			TEXT("Backward"), TEXT("BackwardLeft"), TEXT("Left"), TEXT("ForwardLeft")
		};

		int32 Sector = FMath::RoundToInt(AngleDeg / 45.f) % 8;
		if (Sector < 0)
		{
			Sector += 8;
		}
		return MakeMoveTag(DirectionNames8[Sector]);
	}

	static const TCHAR* DirectionNames4[] = {
		TEXT("Forward"), TEXT("Right"), TEXT("Backward"), TEXT("Left")
	};

	int32 Sector = FMath::RoundToInt(AngleDeg / 90.f) % 4;
	if (Sector < 0)
	{
		Sector += 4;
	}
	return MakeMoveTag(DirectionNames4[Sector]);
}

bool UKnsComboComponent::TryStartCombo(UKnsComboTreeData* Tree, FGameplayTag InputTag)
{
	if (!Tree || !InputTag.IsValid() || IsComboActive())
	{
		return false;
	}

	const FComboLink* BestRootLink = FindBestLink(Tree->RootLinks, InputTag);
	if (const FComboLink* UniversalLink = FindBestLink(Tree->UniversalLinks, InputTag))
	{
		// 通用派生也能作为起手；同优先级时 RootLinks 优先
		if (!BestRootLink || UniversalLink->Priority < BestRootLink->Priority)
		{
			BestRootLink = UniversalLink;
		}
	}

	if (!BestRootLink)
	{
		return false;
	}

	const FComboNode* Node = Tree->FindNode(BestRootLink->TargetNodeId);
	if (!Node)
	{
		return false;
	}

	ActiveTree = Tree;
	return PlayComboMontage(*Node);
}

bool UKnsComboComponent::TryAdvanceCombo(FGameplayTag InputTag)
{
	if (!IsComboActive() || !bComboWindowOpen || !InputTag.IsValid())
	{
		return false;
	}

	const FComboNode* CurrentNode = ActiveTree->FindNode(CurrentNodeId);
	if (!CurrentNode)
	{
		EndCombo(true);
		return false;
	}

	const FComboLink* Link = FindBestLink(CurrentNode->Links, InputTag);
	if (const FComboLink* UniversalLink = FindBestLink(ActiveTree->UniversalLinks, InputTag))
	{
		// 任意节点都能走通用派生；同优先级时节点自身 Links 优先
		if (!Link || UniversalLink->Priority < Link->Priority)
		{
			Link = UniversalLink;
		}
	}

	if (!Link)
	{
		return false;
	}

	const FComboNode* NextNode = ActiveTree->FindNode(Link->TargetNodeId);
	if (!NextNode)
	{
		return false;
	}

	const bool bAdvanced = PlayComboMontage(*NextNode);
	if (bAdvanced)
	{
		LogCombatEvent(TEXT("ComboAdvanced"), FColor::Green, FString::Printf(TEXT("Next=%s"), *NextNode->NodeId.ToString()));
	}
	return bAdvanced;
}

void UKnsComboComponent::AddHitConditionTag(FGameplayTag Tag)
{
	if (!IsComboActive() || !Tag.IsValid() || ActiveConditionTags.HasTagExact(Tag))
	{
		return;
	}

	ActiveConditionTags.AddTag(Tag);
	LogCombatEvent(TEXT("TouchCondition"), FColor::Cyan, Tag.ToString());

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		ASC->AddLooseGameplayTag(Tag);
	}

	TryAutoAdvanceCombo();
}

bool UKnsComboComponent::TryAutoAdvanceCombo()
{
	if (!IsComboActive())
	{
		return false;
	}

	const FComboNode* CurrentNode = ActiveTree->FindNode(CurrentNodeId);
	if (!CurrentNode)
	{
		EndCombo(true);
		return false;
	}

	const FComboLink* Link = FindBestLink(CurrentNode->Links, FGameplayTag());
	if (const FComboLink* UniversalLink = FindBestLink(ActiveTree->UniversalLinks, FGameplayTag()))
	{
		if (!Link || UniversalLink->Priority < Link->Priority)
		{
			Link = UniversalLink;
		}
	}

	if (!Link)
	{
		return false;
	}

	const FComboNode* NextNode = ActiveTree->FindNode(Link->TargetNodeId);
	if (!NextNode)
	{
		return false;
	}

	return PlayComboMontage(*NextNode);
}

void UKnsComboComponent::SetComboWindowOpen(bool bOpen)
{
	bComboWindowOpen = bOpen;
	LogCombatEvent(bOpen ? TEXT("ComboWindowOpen") : TEXT("ComboWindowClose"), FColor::Orange, CurrentNodeId.ToString());

	if (bOpen)
	{
		ConsumeBufferedInput();
	}
	else
	{
		PendingInputTag = FGameplayTag::EmptyTag;
	}
}

void UKnsComboComponent::CancelCombo(bool bStopMontage)
{
	if (!IsComboActive())
	{
		return;
	}

	CurrentMontage = nullptr;
	LogCombatEvent(TEXT("ComboCancelled"), FColor::Red, TEXT(""));

	if (bStopMontage)
	{
		if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
		{
			if (UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance())
			{
				AnimInstance->StopAllMontages(0.1f);
			}
		}
	}

	EndCombo(true);
}

bool UKnsComboComponent::IsComboActive() const
{
	return ActiveTree != nullptr && !CurrentNodeId.IsNone();
}

bool UKnsComboComponent::GetCurrentNode(FComboNode& OutNode) const
{
	if (!ActiveTree)
	{
		return false;
	}

	const FComboNode* Node = ActiveTree->FindNode(CurrentNodeId);
	if (!Node)
	{
		return false;
	}

	OutNode = *Node;
	return true;
}

UKnsComboTreeData* UKnsComboComponent::GetActiveTree() const
{
	return ActiveTree;
}

FName UKnsComboComponent::GetCurrentNodeId() const
{
	return CurrentNodeId;
}

UKnsMoveDefinition* UKnsComboComponent::GetCurrentMoveDefinition() const
{
	return ActiveMoveDefinition;
}

int32 UKnsComboComponent::GetBaseMovePoiseLevel() const
{
	return BaseMovePoiseLevel;
}

int32 UKnsComboComponent::GetEffectivePoiseLevel() const
{
	return EffectivePoiseLevel;
}

void UKnsComboComponent::SetSuperArmor(bool bActive)
{
	bSuperArmorActive = bActive;
	EffectivePoiseLevel = bActive ? 99 : BaseMovePoiseLevel;
	OnPoiseChanged.Broadcast(BaseMovePoiseLevel, EffectivePoiseLevel);
	if (BoundCombatContext)
	{
		BoundCombatContext->SetPoiseState(BaseMovePoiseLevel, EffectivePoiseLevel);
	}
	LogCombatEvent(bActive ? TEXT("SuperArmorOn") : TEXT("SuperArmorOff"), FColor::Magenta, FString::Printf(TEXT("Poise=%d"), EffectivePoiseLevel));
}

bool UKnsComboComponent::PlayComboMontage(const FComboNode& Node)
{
	UKnsMoveDefinition* Move = Node.Move.LoadSynchronous();
	if (!Move)
	{
		return false;
	}

	UAnimMontage* Montage = Move->Montage.LoadSynchronous();
	if (!Montage)
	{
		return false;
	}

	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!Character)
	{
		return false;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (ASC && !ActiveNodeGrantedTags.IsEmpty())
	{
		ASC->RemoveLooseGameplayTags(ActiveNodeGrantedTags);
		ActiveNodeGrantedTags.Reset();
	}

	// 进入新节点时清掉上一节点残留的命中条件
	ClearConditionTags();

	CurrentMontage = Montage;
	bComboWindowOpen = false;
	PendingInputTag = FGameplayTag::EmptyTag;

	const float Duration = Character->PlayAnimMontage(Montage, Move->PlayRate, Move->SectionName);
	if (Duration <= 0.f)
	{
		CurrentMontage = nullptr;
		return false;
	}

	if (ASC && !Move->GrantedTags.IsEmpty())
	{
		ASC->AddLooseGameplayTags(Move->GrantedTags);
		ActiveNodeGrantedTags = Move->GrantedTags;
	}

	// 自动维护攻击中 Tag，供取消/受击等系统用 ASC Tag 解耦判断
	if (ASC && !bAttackingTagAdded)
	{
		ASC->AddLooseGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("State.Player.Attacking"), false));
		bAttackingTagAdded = true;
	}

	ActiveMoveDefinition = Move;
	BaseMovePoiseLevel = Move->PoiseLevel;
	EffectivePoiseLevel = Move->PoiseLevel;
	bSuperArmorActive = false;
	CurrentNodeId = Node.NodeId;
	OnComboNodeStarted.Broadcast(ActiveTree, CurrentNodeId);
	OnMoveStarted.Broadcast(Move, CurrentNodeId);
	OnComboActiveChanged.Broadcast(true);
	OnPoiseChanged.Broadcast(BaseMovePoiseLevel, EffectivePoiseLevel);
	if (BoundCombatContext)
	{
		BoundCombatContext->SetMoveState(Move, CurrentNodeId, BaseMovePoiseLevel, EffectivePoiseLevel, true);
	}
	LogCombatEvent(TEXT("ComboNodeStarted"), FColor::Green, FString::Printf(TEXT("Node=%s Move=%s"), *CurrentNodeId.ToString(), *Move->GetName()));
	return true;
}

bool UKnsComboComponent::LinkMatches(const FComboLink& Link, const FGameplayTag& InputTag) const
{
	if (!Link.bAutoTrigger && !Link.InputTag.MatchesTagExact(InputTag))
	{
		return false;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();

	if (!Link.RequiredTags.IsEmpty() && (!ASC || !ASC->HasAllMatchingGameplayTags(Link.RequiredTags)))
	{
		return false;
	}

	if (!Link.BlockedTags.IsEmpty() && ASC && ASC->HasAnyMatchingGameplayTags(Link.BlockedTags))
	{
		return false;
	}

	return true;
}

const FComboLink* UKnsComboComponent::FindBestLink(const TArray<FComboLink>& Links, const FGameplayTag& InputTag) const
{
	const FComboLink* BestLink = nullptr;

	for (const FComboLink& Link : Links)
	{
		// 数字越小越优先：0 最高，其次 1、2...
		if (LinkMatches(Link, InputTag) && (!BestLink || Link.Priority < BestLink->Priority))
		{
			BestLink = &Link;
		}
	}

	return BestLink;
}

void UKnsComboComponent::ConsumeBufferedInput()
{
	if (!PendingInputTag.IsValid() || !bComboWindowOpen || !IsComboActive())
	{
		return;
	}

	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	if (InputBufferTime > 0.f && Now - PendingInputTime > InputBufferTime)
	{
		PendingInputTag = FGameplayTag::EmptyTag;
		return;
	}

	const FGameplayTag BufferedInput = PendingInputTag;
	PendingInputTag = FGameplayTag::EmptyTag;
	TryAdvanceCombo(BufferedInput);
}

void UKnsComboComponent::ClearConditionTags()
{
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		if (!ActiveConditionTags.IsEmpty())
		{
			ASC->RemoveLooseGameplayTags(ActiveConditionTags);
		}
	}

	ActiveConditionTags.Reset();
}

void UKnsComboComponent::EndCombo(bool bCancelled)
{
	if (!ActiveTree && CurrentNodeId.IsNone())
	{
		return;
	}

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		if (!ActiveNodeGrantedTags.IsEmpty())
		{
			ASC->RemoveLooseGameplayTags(ActiveNodeGrantedTags);
		}
	}

	ActiveNodeGrantedTags.Reset();
	ActiveTree = nullptr;
	CurrentNodeId = NAME_None;
	CurrentMontage = nullptr;
	bComboWindowOpen = false;
	PendingInputTag = FGameplayTag::EmptyTag;
	ActiveMoveDefinition = nullptr;
	BaseMovePoiseLevel = 0;
	EffectivePoiseLevel = 0;
	bSuperArmorActive = false;
	ClearConditionTags();
	OnComboActiveChanged.Broadcast(false);
	OnPoiseChanged.Broadcast(0, 0);
	if (BoundCombatContext)
	{
		BoundCombatContext->ClearCombatState();
	}
	LogCombatEvent(TEXT("ComboEnded"), FColor::Red, bCancelled ? TEXT("Cancelled") : TEXT("Finished"));

	if (bAttackingTagAdded)
	{
		if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
		{
			ASC->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("State.Player.Attacking"), false));
		}
		bAttackingTagAdded = false;
	}

	OnComboEnded.Broadcast(bCancelled);
}

UAbilitySystemComponent* UKnsComboComponent::GetAbilitySystemComponent() const
{
	if (AActor* Owner = GetOwner())
	{
		if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Owner))
		{
			return ASI->GetAbilitySystemComponent();
		}
	}

	return nullptr;
}

USkeletalMeshComponent* UKnsComboComponent::GetOwnerMesh() const
{
	if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		return Character->GetMesh();
	}

	if (AActor* Owner = GetOwner())
	{
		return Owner->FindComponentByClass<USkeletalMeshComponent>();
	}

	return nullptr;
}

void UKnsComboComponent::HandleCancelRequested()
{
	CancelCombo(true);
}

void UKnsComboComponent::HandleHitConditionTagRequested(FGameplayTag Tag)
{
	AddHitConditionTag(Tag);
}

void UKnsComboComponent::LogCombatEvent(const FString& EventName, const FColor& Color, const FString& Payload)
{
	if (UWorld* World = GetWorld())
	{
		if (UKnsCombatDebugSubsystem* DebugSubsystem = World->GetSubsystem<UKnsCombatDebugSubsystem>())
		{
			DebugSubsystem->LogEvent(EventName, Color, Payload);
		}
	}
}

void UKnsComboComponent::HandleMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage != CurrentMontage)
	{
		return;
	}

	EndCombo(false);
}
