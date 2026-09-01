#include "RHProjectileGameplayAbility.h"

#include "AbilitySystemComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Demo/Component/KnsTargetLockComponent.h"
#include "Demo/GAS/KnsAbilitySystemComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"

URHProjectileGameplayAbility::URHProjectileGameplayAbility()
{
	// 不填 ProjectileClass 时也直接生成 C++ 基础投射物，BP 子类可覆盖外观。
	ProjectileClass = ARHProjectile::StaticClass();
}

void URHProjectileGameplayAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
	UWorld* World = Avatar ? Avatar->GetWorld() : nullptr;
	if (!World || !ProjectileClass || !Avatar)
	{
		UE_LOG(LogTemp, Warning, TEXT("RH Projectile GA: activate failed (world=%s class=%s avatar=%s)"),
			World ? TEXT("ok") : TEXT("null"),
			*GetNameSafe(ProjectileClass),
			Avatar ? *Avatar->GetName() : TEXT("null"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	USkeletalMeshComponent* Mesh = nullptr;
	if (ACharacter* Character = Cast<ACharacter>(Avatar))
	{
		Mesh = Character->GetMesh();
	}
	else
	{
		Mesh = Avatar->FindComponentByClass<USkeletalMeshComponent>();
	}

	if (!Mesh)
	{
		UE_LOG(LogTemp, Warning, TEXT("RH Projectile GA: avatar has no skeletal mesh"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	FTransform SpawnTransform = Mesh->GetSocketTransform(SpawnSocket, RTS_World);

	FVector AimDirection = Avatar->GetActorForwardVector();
	bool bUsingLockedTarget = false;
	if (UKnsTargetLockComponent* Lock = Avatar->FindComponentByClass<UKnsTargetLockComponent>())
	{
		FVector TargetLocation;
		if (Lock->GetLockedTargetLocation(TargetLocation))
		{
			// 直接瞄准锁定目标位置（GetActorLocation），水平分量避免瞄脚底产生的向下俯冲。
			AimDirection = (TargetLocation - SpawnTransform.GetLocation()).GetSafeNormal2D();
			bUsingLockedTarget = true;
		}
	}
	if (AimDirection.IsNearlyZero())
	{
		AimDirection = Avatar->GetActorForwardVector();
	}

	// 把方向写进生成旋转,让投射物模型朝向与飞行方向一致
	// (飞行方向 = AimDirection,由 Velocity 驱动;旋转必须同源,否则斜飞/乱飞)。
	SpawnTransform.SetRotation(AimDirection.Rotation().Quaternion());

	ARHProjectile* Projectile = World->SpawnActorDeferred<ARHProjectile>(
		ProjectileClass,
		SpawnTransform,
		Avatar,
		Cast<APawn>(Avatar),
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (Projectile)
	{
		UKnsAbilitySystemComponent* SourceASC = Cast<UKnsAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
		Projectile->Initialize(Speed, MaxLifetime, Damage, ResonanceDamage, PoiseLevel, Avatar, SourceASC, AimDirection);
		Projectile->FinishSpawning(SpawnTransform);
		// 组件注册完成后再设一次飞行方向:deferred spawn 期间设置 Velocity 存在被
		// 引擎首次 Tick 覆盖的时序风险,注册后设置才是绝对可靠的。
		Projectile->SetMovementDirection(AimDirection);
		UE_LOG(LogTemp, Warning, TEXT("RH Projectile GA: spawned %s from %s (locked=%d dir=%s socket=%s)"),
			*Projectile->GetName(), *Avatar->GetName(), bUsingLockedTarget ? 1 : 0, *AimDirection.ToString(), *SpawnSocket.ToString());
	}
	else
	{
		const AActor* ClassDefault = ProjectileClass ? ProjectileClass->GetDefaultObject<AActor>() : nullptr;
		const USceneComponent* DefaultRoot = ClassDefault ? ClassDefault->GetRootComponent() : nullptr;
		UE_LOG(LogTemp, Warning, TEXT("RH Projectile GA: failed to spawn %s (transform=%s socket=%s world=%s level=%s default=%s default_root=%s)"),
			*GetNameSafe(ProjectileClass),
			*SpawnTransform.ToString(),
			*SpawnSocket.ToString(),
			World ? *World->GetName() : TEXT("null"),
			World && World->GetCurrentLevel() ? *World->GetCurrentLevel()->GetName() : TEXT("null"),
			ClassDefault ? *ClassDefault->GetClass()->GetName() : TEXT("null"),
			DefaultRoot ? *DefaultRoot->GetClass()->GetName() : TEXT("null"));
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
}
