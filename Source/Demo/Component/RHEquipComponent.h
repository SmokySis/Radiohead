#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Demo/Combat/Weapon/AWeaponBase.h"
#include "Demo/Onom/RHOnomActionDefinition.h"
#include "Demo/Onom/RHWeaponDefinition.h"
#include "RHEquipComponent.generated.h"

class UAnimInstance;
class URHCoreDefinition;
class UKnsCombatComponent;
class URHCombatComponent;
class URHOnomComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRHWeaponSwitched, int32, WeaponIndex);

/** 一个武器槽：战斗数据 DA（武器类/主手 Socket 都从 DA 取）、联动动画层。 */
USTRUCT(BlueprintType)
struct FRHWeaponSlot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	FName SlotId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	TObjectPtr<URHWeaponDefinition> WeaponDefinition;

	/** 武器联动动画层（Link Anim Class），切换武器时同步链接/解链。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	TSubclassOf<UAnimInstance> AnimLayerClass;
};

/**
 * RH 装备组件：武器槽（DA/动画层，武器类与主手 Socket 从 DA 取）、音律核心。
 * 后续饰品槽在此扩展；战斗组件只保留底层 SwitchWeapon 原语。
 */
UCLASS(ClassGroup = (RadioHead), meta = (BlueprintSpawnableComponent))
class DEMO_API URHEquipComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	URHEquipComponent();

	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equip")
	TArray<FRHWeaponSlot> WeaponSlots;

	/** 音律武器列表（统一动作 DA，X 切换/HandleRhythmWeapon 按 ID 调用）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equip")
	TArray<TObjectPtr<URHOnomActionDefinition>> RhythmWeapons;

	/** 音律核心：决定受伤时的音形结算（失去/增加/清空）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equip")
	TObjectPtr<URHCoreDefinition> OnomCore;

	UPROPERTY(BlueprintAssignable, Category = "Equip|Events")
	FRHWeaponSwitched OnWeaponSwitched;

	/** 普通切换武器：应用槽位的类/Socket/DA/动画层。 */
	UFUNCTION(BlueprintCallable, Category = "Equip")
	bool SwitchToWeapon(int32 WeaponIndex);

	/** 切换武器并立刻使出切换技（新武器 DA 的 SwitchTactic）。 */
	UFUNCTION(BlueprintCallable, Category = "Equip")
	bool SwitchToWeaponWithTactic(int32 WeaponIndex);

	/** 授予所有音律武器的效果 GA（懒授予兜底，BeginPlay 调用）。 */
	UFUNCTION(BlueprintCallable, Category = "Equip")
	void GrantRhythmWeaponAbilities();

	UFUNCTION(BlueprintPure, Category = "Equip")
	int32 GetCurrentWeaponIndex() const;

	UFUNCTION(BlueprintPure, Category = "Equip")
	URHWeaponDefinition* GetCurrentWeaponDefinition() const;

	/** 装备音律核心并同步到 Onom 组件。 */
	UFUNCTION(BlueprintCallable, Category = "Equip")
	void SetOnomCore(URHCoreDefinition* Core);

	UFUNCTION(BlueprintPure, Category = "Equip")
	URHCoreDefinition* GetOnomCore() const;

protected:
	void ApplyWeaponSlot(int32 WeaponIndex);
	void ApplyOnomCore();
	bool IsInFreeState() const;
	bool IsSpecialCancelActive() const;

	UPROPERTY(Transient)
	int32 CurrentWeaponIndex = INDEX_NONE;

	UPROPERTY(Transient)
	TSubclassOf<UAnimInstance> CurrentAnimLayerClass;
};
