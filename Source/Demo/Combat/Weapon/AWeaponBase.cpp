#include "AWeaponBase.h"

#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "NiagaraComponent.h"

AWeaponBase::AWeaponBase()
{
	PrimaryActorTick.bCanEverTick = false;

	WeaponRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = WeaponRoot;

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(WeaponRoot);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	WeaponStaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponStaticMesh"));
	WeaponStaticMesh->SetupAttachment(WeaponMesh);
	WeaponStaticMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	WeaponTrail = CreateDefaultSubobject<UNiagaraComponent>(TEXT("WeaponTrail"));
	WeaponTrail->SetupAttachment(WeaponStaticMesh);
	WeaponTrail->SetAutoActivate(false);

	HitboxBox = CreateDefaultSubobject<UBoxComponent>(TEXT("HitboxBox"));
	HitboxBox->SetupAttachment(WeaponMesh);
	HitboxBox->SetBoxExtent(FVector(5.f, 5.f, 10.f));
	HitboxBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	HitboxBox->SetGenerateOverlapEvents(true);
	HitboxBox->SetCollisionObjectType(ECC_GameTraceChannel1);
	HitboxBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	HitboxBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	HitboxBox->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Overlap);

	// 双刀：固定副刀身 + 副命中框（副刀与主刀同一套命中状态，命中/相杀/Blitz 全生效）。
	SecondaryMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SecondaryMesh"));
	SecondaryMesh->SetupAttachment(WeaponRoot);
	SecondaryMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	SecondaryHitboxBox = CreateDefaultSubobject<UBoxComponent>(TEXT("SecondaryHitboxBox"));
	SecondaryHitboxBox->SetupAttachment(SecondaryMesh);
	SecondaryHitboxBox->SetBoxExtent(FVector(5.f, 5.f, 10.f));
	SecondaryHitboxBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SecondaryHitboxBox->SetGenerateOverlapEvents(true);
	SecondaryHitboxBox->SetCollisionObjectType(ECC_GameTraceChannel1);
	SecondaryHitboxBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	SecondaryHitboxBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	SecondaryHitboxBox->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Overlap);
}

UBoxComponent* AWeaponBase::GetHitboxBox() const
{
	return HitboxBox;
}

UBoxComponent* AWeaponBase::GetSecondaryHitboxBox() const
{
	return SecondaryHitboxBox;
}

void AWeaponBase::AttachSecondaryMesh(USkeletalMeshComponent* CharacterMesh)
{
	// socket 未配置（None）= 不绑定副刀：即使蓝图误配了副刀静态网格资产也不显示。
	if (!CharacterMesh || SecondarySocketName.IsNone())
	{
		if (SecondaryMesh)
		{
			SecondaryMesh->SetVisibility(false);
		}
		return;
	}

	// 副刀资产未配置时复用主静态网格（双刀默认同款刀身）；主网格也没有则无法挂副刀。
	if (SecondaryMesh && !SecondaryMesh->GetStaticMesh())
	{
		if (WeaponStaticMesh)
		{
			SecondaryMesh->SetStaticMesh(WeaponStaticMesh->GetStaticMesh());
		}
		if (!SecondaryMesh->GetStaticMesh())
		{
			UE_LOG(LogTemp, Warning, TEXT("RH Weapon: no static mesh for secondary blade on %s"), *GetName());
			return;
		}
	}

	// 跨 Actor attach 到角色骨骼网格的副 socket，SnapToTarget 保证挂点变换来自 socket。
	SecondaryMesh->AttachToComponent(CharacterMesh,
		FAttachmentTransformRules::SnapToTargetNotIncludingScale, SecondarySocketName);
	SecondaryMesh->SetVisibility(true);
	UE_LOG(LogTemp, Log, TEXT("RH Weapon: attached secondary mesh to socket %s"), *SecondarySocketName.ToString());
}

void AWeaponBase::ActivateWeaponTrail()
{
	if (WeaponTrail)
	{
		WeaponTrail->Activate(true);
	}
}

void AWeaponBase::DeactivateWeaponTrail()
{
	if (WeaponTrail)
	{
		WeaponTrail->Deactivate();
	}
}
