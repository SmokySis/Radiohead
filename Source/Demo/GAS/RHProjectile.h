#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RHProjectile.generated.h"

class AActor;
class UBoxComponent;
class UProjectileMovementComponent;
class UStaticMeshComponent;
class USceneComponent;
class UKnsAbilitySystemComponent;

UCLASS()
class DEMO_API ARHProjectile : public AActor
{
	GENERATED_BODY()

public:
	ARHProjectile();

	/** 由 GA 在生成后调用:写入飞行与伤害参数(碰撞体积在 Actor/BP 上配置)。
	 *  InDirection 为 ZeroVector(未指定)时不设置速度,由 UProjectileMovementComponent
	 *  首次 Tick 按已应用旋转的组件前向兜底——切勿用 FVector::ForwardVector 当默认值(世界 X 轴)。 */
	UFUNCTION(BlueprintCallable, Category = "Projectile")
	void Initialize(float InSpeed, float InMaxLifetime, float InDamage, float InResonanceDamage, int32 InPoiseLevel, AActor* InSource, UKnsAbilitySystemComponent* InSourceASC, const FVector& InDirection = FVector::ZeroVector);

	/** 组件注册完成后设置飞行方向(速度方向)。零向量忽略。 */
	UFUNCTION(BlueprintCallable, Category = "Projectile")
	void SetMovementDirection(const FVector& InDirection);

protected:
	UFUNCTION()
	void HandleBoxBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleProjectileStop(const FHitResult& ImpactResult);

	void ResolveHit(AActor* HitActor, const FVector& HitLocation);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<USceneComponent> ProjectileRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<UBoxComponent> CollisionBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<UStaticMeshComponent> ProjectileMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<UProjectileMovementComponent> MovementComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Hit", meta = (ClampMin = "0"))
	float Damage = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Hit", meta = (ClampMin = "0"))
	float ResonanceDamage = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Hit", meta = (ClampMin = "0"))
	int32 PoiseLevel = 1;

	UPROPERTY(Transient)
	TWeakObjectPtr<UKnsAbilitySystemComponent> SourceASC;

	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> SourceActor;
};
