#include "RHTestDummy.h"

#include "Components/BoxComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Demo/AI/RHEnemyAIComponent.h"
#include "Demo/AI/RHEnemyCombatComponent.h"
#include "Demo/Combat/RHCombatComponent.h"
#include "Demo/Combat/RHHitData.h"
#include "GameFramework/Pawn.h"
#include "Materials/MaterialInterface.h"

ARHTestDummy::ARHTestDummy()
{
	// 需求1：木桩默认不朝向玩家（基类 AI 组件的 TickRotateToPlayer 默认会转向玩家，这里关掉）。
	if (RHEnemyAIComponent)
	{
		RHEnemyAIComponent->SetRotateToPlayer(false);
	}

	// 需求2：受击判定框。默认 NoCollision（不参与检测），启用时切 QueryOnly + 记录重叠。
	ProbeBox = CreateDefaultSubobject<UBoxComponent>(TEXT("RH_ProbeBox"));
	ProbeBox->SetupAttachment(GetRootComponent());
	ProbeBox->SetBoxExtent(FVector(100.f, 100.f, 100.f));
	ProbeBox->SetRelativeLocation(FVector(0.f, 0.f, 60.f));
	ProbeBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ProbeBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	// 关键：单独放行 Pawn 通道（玩家胶囊体所在通道），否则启用后与玩家不产生重叠、检测永远为空。
	ProbeBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	ProbeBox->SetGenerateOverlapEvents(true);
	UpdateProbeBoxVisual();
}

void ARHTestDummy::BeginPlay()
{
	Super::BeginPlay();
	ApplyProbeEnabledState();
}

void ARHTestDummy::SetProbeEnabled(bool bEnabled)
{
	if (bProbeEnabled == bEnabled)
	{
		return;
	}
	bProbeEnabled = bEnabled;
	ApplyProbeEnabledState();
}

void ARHTestDummy::ApplyProbeEnabledState()
{
	if (!ProbeBox)
	{
		return;
	}

	// 判定框：启用时 QueryOnly + 记录重叠（周期检测读 GetOverlappingActors），关闭时 NoCollision。
	ProbeBox->SetCollisionEnabled(bProbeEnabled ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
	ProbeBox->SetGenerateOverlapEvents(bProbeEnabled);
	// 与玩家 capsule 保持重叠响应（构造已设，这里防御性重申，防止被其它逻辑改掉）。
	ProbeBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	UE_LOG(LogTemp, Warning, TEXT("RHTestDummy: probe %s (interval=%.2f, damage=%.1f, resonance=%.1f, poise=%d)"),
		bProbeEnabled ? TEXT("ENABLED") : TEXT("disabled"),
		ProbeCheckInterval, ProbeDamage, ProbeResonanceDamage, ProbePoiseLevel);

	// 开启时给木桩 mesh 上 overlay 材质（用户填），关闭时清除。
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		MeshComp->SetOverlayMaterial(bProbeEnabled ? ProbeActiveOverlayMaterial : nullptr);
	}

	UpdateProbeBoxVisual();

	// 周期检测定时器（间隔变更需重新开关判定框生效）。
	if (UWorld* World = GetWorld())
	{
		if (bProbeEnabled)
		{
			if (!ProbeTimerHandle.IsValid())
			{
				World->GetTimerManager().SetTimer(
					ProbeTimerHandle,
					this,
					&ARHTestDummy::TickProbe,
					FMath::Max(ProbeCheckInterval, 0.05f),
					true);
			}
		}
		else
		{
			World->GetTimerManager().ClearTimer(ProbeTimerHandle);
		}
	}
}

void ARHTestDummy::UpdateProbeBoxVisual()
{
	if (ProbeBox)
	{
		ProbeBox->SetHiddenInGame(!bShowProbeBoxInGame);
		ProbeBox->SetVisibility(bShowProbeBoxInGame, true);
	}
}

void ARHTestDummy::TickProbe()
{
	if (!bProbeEnabled || !ProbeBox || !RHEnemyCombatComponent)
	{
		return;
	}

	TArray<AActor*> Overlapped;
	ProbeBox->GetOverlappingActors(Overlapped, APawn::StaticClass());
	for (AActor* Actor : Overlapped)
	{
		if (!Actor || Actor == this)
		{
			continue;
		}
		// 只判定玩家：有 RH 战斗组件的 Pawn（与处决范围检测的判定方式一致）。
		if (!Actor->FindComponentByClass<URHCombatComponent>())
		{
			continue;
		}

		FRHHitData HitData;
		HitData.Damage = ProbeDamage;
		HitData.ResonanceDamage = ProbeResonanceDamage;
		HitData.PoiseLevel = ProbePoiseLevel;
		HitData.Source = this;
		// 命中点取判定框世界位置：受击方向按“命中点相对玩家”计算（框在木桩身上 → 面朝木桩）。
		HitData.HitLocation = ProbeBox->GetComponentLocation();

		if (RHEnemyCombatComponent->ApplyHitToTarget(Actor, HitData))
		{
			UE_LOG(LogTemp, Warning, TEXT("RHTestDummy: probe hit %s (Damage=%.1f Resonance=%.1f Poise=%d)"),
				*Actor->GetName(), HitData.Damage, HitData.ResonanceDamage, HitData.PoiseLevel);
		}
	}
}
