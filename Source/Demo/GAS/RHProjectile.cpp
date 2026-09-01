#include "RHProjectile.h"

#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Demo/Combat/RHHitData.h"
#include "Demo/GAS/KnsAbilitySystemComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/ProjectileMovementComponent.h"

ARHProjectile::ARHProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	ProjectileRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultSceneRoot"));
	RootComponent = ProjectileRoot;

	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	CollisionBox->SetupAttachment(ProjectileRoot);
	CollisionBox->SetBoxExtent(FVector(15.f));
	CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionBox->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionBox->SetCollisionResponseToAllChannels(ECR_Overlap);
	CollisionBox->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	CollisionBox->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	CollisionBox->SetGenerateOverlapEvents(true);
	CollisionBox->OnComponentBeginOverlap.AddDynamic(this, &ARHProjectile::HandleBoxBeginOverlap);

	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileMesh"));
	ProjectileMesh->SetupAttachment(CollisionBox);
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	MovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("MovementComponent"));
	MovementComponent->UpdatedComponent = CollisionBox;
	MovementComponent->InitialSpeed = 1500.f;
	MovementComponent->MaxSpeed = 1500.f;
	MovementComponent->bRotationFollowsVelocity = false;
	MovementComponent->bShouldBounce = false;
	MovementComponent->ProjectileGravityScale = 0.f;
	MovementComponent->OnProjectileStop.AddDynamic(this, &ARHProjectile::HandleProjectileStop);
}

void ARHProjectile::Initialize(float InSpeed, float InMaxLifetime, float InDamage, float InResonanceDamage, int32 InPoiseLevel, AActor* InSource, UKnsAbilitySystemComponent* InSourceASC, const FVector& InDirection)
{
	MovementComponent->InitialSpeed = FMath::Max(InSpeed, 1.f);
	MovementComponent->MaxSpeed = MovementComponent->InitialSpeed;
	const FVector Direction = InDirection.GetSafeNormal();
	if (!Direction.IsNearlyZero())
	{
		MovementComponent->Velocity = Direction * MovementComponent->InitialSpeed;
	}
	// Direction 为零(调用方未指定)时不设置 Velocity:
	// UProjectileMovementComponent 首次 Tick 会用已注册 UpdatedComponent 的前向兜底,
	// 而 FinishSpawning 后该前向等于 SpawnTransform 的旋转(GA 已写入 AimDirection)。
	// 注意:本函数在 FinishSpawning 之前调用,此时 GetActorRotation()/组件前向都是
	// identity(世界 X 轴),绝不能在此时读取它们做兜底。
	Damage = FMath::Max(InDamage, 0.f);
	ResonanceDamage = FMath::Max(InResonanceDamage, 0.f);
	PoiseLevel = FMath::Max(InPoiseLevel, 0);
	SourceASC = InSourceASC;
	SourceActor = InSource;

	if (InSource)
	{
		SetOwner(InSource);
		SetInstigator(Cast<APawn>(InSource));
		CollisionBox->IgnoreActorWhenMoving(InSource, true);
	}

	if (InMaxLifetime > 0.f)
	{
		SetLifeSpan(InMaxLifetime);
	}
}

void ARHProjectile::SetMovementDirection(const FVector& InDirection)
{
	const FVector Direction = InDirection.GetSafeNormal();
	if (!Direction.IsNearlyZero())
	{
		MovementComponent->Velocity = Direction * MovementComponent->InitialSpeed;
	}
}

void ARHProjectile::HandleBoxBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	FVector HitLocation = OtherActor ? OtherActor->GetActorLocation() : FVector::ZeroVector;
	if (OverlappedComponent && OtherActor)
	{
		FVector ClosestPoint;
		if (OverlappedComponent->GetClosestPointOnCollision(OtherActor->GetActorLocation(), ClosestPoint) >= 0.f)
		{
			HitLocation = ClosestPoint;
		}
	}

	ResolveHit(OtherActor, HitLocation);
}

void ARHProjectile::HandleProjectileStop(const FHitResult& ImpactResult)
{
	ResolveHit(ImpactResult.GetActor(), ImpactResult.ImpactPoint);
}

void ARHProjectile::ResolveHit(AActor* HitActor, const FVector& HitLocation)
{
	if (HitActor)
	{
		AActor* Source = SourceActor.Get();
		if (!Source || HitActor == Source || HitActor->IsOwnedBy(Source) || HitActor->IsA<ARHProjectile>())
		{
			return;
		}

		if (SourceASC.IsValid())
		{
			FRHHitData HitData;
			HitData.Damage = Damage;
			HitData.ResonanceDamage = ResonanceDamage;
			HitData.PoiseLevel = PoiseLevel;
			HitData.Source = Source;
			HitData.HitLocation = HitLocation.IsNearlyZero() ? GetActorLocation() : HitLocation;
			HitData.bIsSkill = true;
			HitData.bApplyOnomPenalty = false;
			SourceASC->ApplyHitToActor(HitActor, HitData);
		}
	}

	Destroy();
}
