#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AWeaponBase.generated.h"

class UBoxComponent;
class USceneComponent;
class USkeletalMeshComponent;
class UStaticMeshComponent;
class UNiagaraComponent;

UCLASS()
class DEMO_API AWeaponBase : public AActor
{
	GENERATED_BODY()

public:
	AWeaponBase();

	/** 武器根节点 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<USceneComponent> WeaponRoot;

	/** 武器外观网格 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<USkeletalMeshComponent> WeaponMesh;

	/** 附属静态网格体（子类武器模型挂这里；与骨骼网格体同时上下覆层材质）。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<UStaticMeshComponent> WeaponStaticMesh;

	/** 武器拖尾 Niagara 组件（挂在静态网格体下，parent socket 在编辑器里配置；ANS 只负责换资产与显隐）。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<UNiagaraComponent> WeaponTrail;

	/** 武器命中判定盒（Box，替代旧 Capsule） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<UBoxComponent> HitboxBox;

	/** 副刀外观网格（双刀武器）：固定第二把刀身，挂到角色 Mesh 的副 socket，与主刀一样参与命中判定。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<UStaticMeshComponent> SecondaryMesh;

	/** 副刀命中判定盒：与主 HitboxBox 共用同一套命中状态（ActiveHitboxTag/HitActorsThisHitbox），命中、相杀、Blitz 全部生效。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<UBoxComponent> SecondaryHitboxBox;

	/** 副刀挂载的 Mesh Socket 名。默认 None = 不绑定副刀（单刀武器）；双刀武器在资产里填具体 socket（如 weapon_l）后才会挂载副刀。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	FName SecondarySocketName = NAME_None;

	UFUNCTION(BlueprintPure, Category = "Weapon")
	UBoxComponent* GetHitboxBox() const;

	UFUNCTION(BlueprintPure, Category = "Weapon")
	UBoxComponent* GetSecondaryHitboxBox() const;

	/** 是否配置了副手 socket（True=双刀，副刀身与副命中框才会挂载/参与判定；False=单刀，副刀不显示也不判定）。 */
	UFUNCTION(BlueprintPure, Category = "Weapon")
	bool HasSecondaryBlade() const { return !SecondarySocketName.IsNone(); }

	/** 双刀：把副刀挂到角色 Mesh 的副 socket（SecondarySocketName）。副刀静态网格资产未配置时复用主刀外观。 */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void AttachSecondaryMesh(USkeletalMeshComponent* CharacterMesh);

	/** 激活武器拖尾（战技施放反馈；具体 Niagara 资产在编辑器里配）。 */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void ActivateWeaponTrail();

	/** 关闭武器拖尾（战技结束/取消/被打断时）。 */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void DeactivateWeaponTrail();
};
