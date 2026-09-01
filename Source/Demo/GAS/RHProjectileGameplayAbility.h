#pragma once

#include "CoreMinimal.h"
#include "Demo/GAS/RHGameplayAbility.h"
#include "Demo/GAS/RHProjectile.h"
#include "RHProjectileGameplayAbility.generated.h"

UCLASS()
class DEMO_API URHProjectileGameplayAbility : public URHGameplayAbility
{
	GENERATED_BODY()

public:
	URHProjectileGameplayAbility();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	TSubclassOf<ARHProjectile> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	FName SpawnSocket = TEXT("Muzzle");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile", meta = (ClampMin = "1"))
	float Speed = 1500.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile", meta = (ClampMin = "0.01"))
	float MaxLifetime = 3.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile", meta = (ClampMin = "0"))
	float Damage = 30.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile", meta = (ClampMin = "0"))
	float ResonanceDamage = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile", meta = (ClampMin = "0"))
	int32 PoiseLevel = 1;
};
