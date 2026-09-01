#include "RHEnemyBase.h"

#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/CapsuleComponent.h"
#include "Blueprint/UserWidget.h"
#include "Demo/AI/RHEnemyAIController.h"
#include "Demo/AI/RHEnemyAIComponent.h"
#include "Demo/AI/RHEnemyCombatComponent.h"
#include "Demo/Combat/RHCombatActionInterface.h"
#include "Demo/Combat/RHCombatComponent.h"
#include "Demo/Combat/KnsCombatContextComponent.h"
#include "Demo/UI/RHEnemyFloatPanelWidget.h"
#include "Demo/UI/RHEnemyPanelWidget.h"

ARHEnemyBase::ARHEnemyBase()
{
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Overlap);
	GetCapsuleComponent()->SetGenerateOverlapEvents(true);

	RHAbilitySystemComponent = CreateDefaultSubobject<UKnsAbilitySystemComponent>(TEXT("RH_Enemy_ASC"));
	RHCommonAttributeSet = CreateDefaultSubobject<UKnsCommonAttributeSet>(TEXT("RH_Enemy_CommonAS"));
	RHCombatContext = CreateDefaultSubobject<UKnsCombatContextComponent>(TEXT("RH_Enemy_CombatContext"));
	RHEnemyCombatComponent = CreateDefaultSubobject<URHEnemyCombatComponent>(TEXT("RH_Enemy_Combat"));
	RHEnemyAIComponent = CreateDefaultSubobject<URHEnemyAIComponent>(TEXT("RH_Enemy_AI"));

	ExecutionRangeSphere = CreateDefaultSubobject<USphereComponent>(TEXT("ExecutionRangeSphere"));
	ExecutionRangeSphere->SetupAttachment(GetRootComponent());
	ExecutionRangeSphere->InitSphereRadius(180.f);
	ExecutionRangeSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ExecutionRangeSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	ExecutionRangeSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	ExecutionRangeSphere->SetGenerateOverlapEvents(true);

	LockIndicatorWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("LockIndicatorWidget"));
	LockIndicatorWidget->SetupAttachment(GetRootComponent());
	LockIndicatorWidget->SetWidgetSpace(EWidgetSpace::World);
	LockIndicatorWidget->SetVisibility(false);

	FloatingHealthBarWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("FloatingHealthBarWidget"));
	FloatingHealthBarWidget->SetupAttachment(GetRootComponent());
	FloatingHealthBarWidget->SetRelativeLocation(FVector(0.f, 0.f, 80.f));
	FloatingHealthBarWidget->SetWidgetSpace(EWidgetSpace::World);
	FloatingHealthBarWidget->SetVisibility(false);

	AIControllerClass = ARHEnemyAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

void ARHEnemyBase::BeginPlay()
{
	Super::BeginPlay();
	RHAbilitySystemComponent->InitAbilityActorInfo(this, this);
	if (ExecutionRangeSphere)
	{
		ExecutionRangeSphere->OnComponentBeginOverlap.AddDynamic(this, &ARHEnemyBase::OnExecutionOverlapBegin);
		ExecutionRangeSphere->OnComponentEndOverlap.AddDynamic(this, &ARHEnemyBase::OnExecutionOverlapEnd);
	}
	BindFloatBarWidget();
}

void ARHEnemyBase::SetExecutionRangeActive(bool bActive)
{
	if (ExecutionRangeSphere)
	{
		ExecutionRangeSphere->SetCollisionEnabled(bActive ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
	}
}

void ARHEnemyBase::NotifyTargetLockChanged(bool bLocked)
{
	SetLockIndicatorVisible(bLocked);
}

void ARHEnemyBase::SetLockIndicatorVisible(bool bVisible)
{
	bIsLocked = bVisible;
	if (LockIndicatorWidget)
	{
		LockIndicatorWidget->SetVisibility(bVisible);
		LockIndicatorWidget->SetHiddenInGame(!bVisible);
	}
	if (FloatingHealthBarWidget)
	{
		const bool bShowBar = bFloatBar && bVisible;
		FloatingHealthBarWidget->SetVisibility(bShowBar);
		FloatingHealthBarWidget->SetHiddenInGame(!bShowBar);
		if (bShowBar)
		{
			BindFloatBarWidget();
		}
	}
}

void ARHEnemyBase::ApplyFloatBarConfig(bool bInFloatBar)
{
	bFloatBar = bInFloatBar;
	if (FloatingHealthBarWidget)
	{
		const bool bShowBar = bFloatBar && bIsLocked;
		FloatingHealthBarWidget->SetVisibility(bShowBar);
		FloatingHealthBarWidget->SetHiddenInGame(!bShowBar);
		if (bShowBar)
		{
			BindFloatBarWidget();
		}
	}
}

void ARHEnemyBase::HideAllUI()
{
	bIsLocked = false;
	if (LockIndicatorWidget)
	{
		LockIndicatorWidget->SetVisibility(false);
		LockIndicatorWidget->SetHiddenInGame(true);
	}
	if (FloatingHealthBarWidget)
	{
		FloatingHealthBarWidget->SetVisibility(false);
		FloatingHealthBarWidget->SetHiddenInGame(true);
	}
}

void ARHEnemyBase::BindFloatBarWidget()
{
	if (!FloatingHealthBarWidget)
	{
		return;
	}
	UUserWidget* Widget = FloatingHealthBarWidget->GetWidget();
	if (!Widget)
	{
		if (!bFloatBarBindQueued && GetWorld())
		{
			bFloatBarBindQueued = true;
			GetWorld()->GetTimerManager().SetTimerForNextTick([this]()
			{
				bFloatBarBindQueued = false;
				BindFloatBarWidget();
			});
		}
		return;
	}
	if (URHEnemyPanelWidget* Panel = Cast<URHEnemyPanelWidget>(Widget))
	{
		Panel->BindTarget(GetOwner());
		return;
	}
	if (URHEnemyFloatPanelWidget* FloatPanel = Cast<URHEnemyFloatPanelWidget>(Widget))
	{
		FloatPanel->BindTarget(GetOwner());
	}
}

void ARHEnemyBase::OnExecutionOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherActor || OtherActor == this || !OtherActor->FindComponentByClass<URHCombatComponent>())
	{
		return;
	}
	if (IRHCombatActionInterface* PlayerInterface = Cast<IRHCombatActionInterface>(OtherActor))
	{
		PlayerInterface->SetExecutionAvailable(true, this);
	}
}

void ARHEnemyBase::OnExecutionOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!OtherActor || OtherActor == this || !OtherActor->FindComponentByClass<URHCombatComponent>())
	{
		return;
	}
	if (IRHCombatActionInterface* PlayerInterface = Cast<IRHCombatActionInterface>(OtherActor))
	{
		PlayerInterface->SetExecutionAvailable(false, nullptr);
	}
}

UAbilitySystemComponent* ARHEnemyBase::GetAbilitySystemComponent() const
{
	return RHAbilitySystemComponent;
}
