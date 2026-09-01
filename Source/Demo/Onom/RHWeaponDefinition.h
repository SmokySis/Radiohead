#pragma once

#include "CoreMinimal.h"
#include "Demo/Combat/KnsHitReactionSettingsDataAsset.h"
#include "Engine/DataAsset.h"
#include "Demo/Combat/RHMoveDefinition.h"
#include "Demo/Onom/RHOnomActionDefinition.h"
#include "Demo/Onom/RHOnomTypes.h"
#include "RHWeaponDefinition.generated.h"

class AWeaponBase;

/** 命中获得模式：攻击命中后获得 onom 音形，还是直接攒共鸣。 */
UENUM(BlueprintType)
enum class ERHOnomHitGainMode : uint8
{
	/** 命中获得 onom 音形（按 AttackHitRule）。 */
	Onom,
	/** 命中获得共鸣：共鸣层 +1（类型按本次命中规则），衰减速率按武器 DA 的 ResonanceDecayRate。 */
	Resonance
};

/** 防御性招式类型：武器 DA 决定防御键的行为。 */
UENUM(BlueprintType)
enum class ERHDefensiveType : uint8
{
	/** 弹反：点按，播 DefensiveMontage（L/R 段），蒙太奇内 RH Parry Window 判定。 */
	Parry,
	/** 防御：按住持续，播 DefensiveMontage 循环段，退出时停止。 */
	Defend,
	/** 逆转逆装填（re-reload）：点按，需共鸣槽，播 DefensiveMontage 的 ReReload 段，蒙太奇内 RH Reverse Just Reload Window 判定。 */
	ReverseReload
};

UCLASS(BlueprintType)
class DEMO_API URHWeaponDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	URHWeaponDefinition();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, AssetRegistrySearchable, Category = "Weapon")
	FName WeaponId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	FText DisplayName;

	/** 武器 Actor 类（实体武器；为空则不生成）。副刀外观/副手 socket 在武器蓝图 AWeaponBase.SecondarySocketName 上配。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Visual")
	TSubclassOf<AWeaponBase> WeaponClass;

	/** 武器挂载的 Mesh Socket 名（主手）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Visual")
	FName WeaponSocketName = TEXT("weapon_r");

	/** 武器喜好属性：共鸣属性与之一致时三种加成全部生效（各乘独立系数）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Onom")
	ERHOnomPolarity PreferredPolarity = ERHOnomPolarity::None;

	/** 伤害独立系数。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Onom", meta = (ClampMin = "0"))
	float DamageCoeff = 1.f;

	/** 共振伤害独立系数。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Onom", meta = (ClampMin = "0"))
	float ResonanceDamageCoeff = 1.f;

	/** 轰鸣获取独立系数。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Onom", meta = (ClampMin = "0"))
	float ChargeCoeff = 1.f;

	/** 共鸣衰减速率倍率：攻击命中攒共鸣（HitGainMode=Resonance）时按此倍率衰减倒计时。每把武器可单独设置，默认 1.0（旧行为硬编码 ×2，现改为可配）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Onom", meta = (ClampMin = "0"))
	float ResonanceDecayRate = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Onom")
	FRHOnomSourceRule AttackHitRule;

	/** 命中获得共鸣：共鸣层 +1（类型按本次命中规则），衰减速率按本 DA 的 ResonanceDecayRate。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Onom")
	ERHOnomHitGainMode HitGainMode = ERHOnomHitGainMode::Onom;

	/** 逆装填武器：勾选后 HandleLoad 从“检测手牌 onom”改为“检测共鸣槽”，装填键走逆装填（共鸣→手牌，蒙太奇上放 RH Reload AN）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Onom")
	bool bReload = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Onom")
	FRHOnomSourceRule NormalGuardHitRule;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Onom")
	FRHOnomSourceRule PerfectGuardHitRule;

	/** 完美防御获得模式：Onom=获得 PerfectGuardHitRule（次音规则）的音形；Resonance=共鸣层+1（类型按次音规则），衰减按 ResonanceDecayRate。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Onom")
	ERHOnomHitGainMode PerfectGuardGainMode = ERHOnomHitGainMode::Onom;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Attack")
	TArray<TObjectPtr<URHMoveDefinition>> Attacks;

	/** 跑步派生攻击：持有 Action.Move.Run 时按攻击键播放的招式。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Attack")
	TObjectPtr<URHMoveDefinition> RunAttackMoveDefinition;

	/** 闪避派生攻击：闪避结束后（连段段数为 0 时）按攻击键播放的特殊招式。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Attack")
	TObjectPtr<URHMoveDefinition> DodgeAttackMoveDefinition;

	/** 闪避不打断普攻连段：闪避后 ComboBridgeTimeout 秒内按普攻从当前段 +1 续。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Combo")
	bool bDodgePreservesCombo = false;

	/** 连段桥有效期（秒）：快速装填/闪避保留段数后，超时未续段则作废（段数归零）。0 = 不超时。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Combo", meta = (ClampMin = "0"))
	float ComboBridgeTimeout = 2.f;

	/** 闪避蒙太奇：4 个 Section 名 F/L/R/B，按移动输入与面朝夹角选择。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Move")
	TObjectPtr<UAnimMontage> DodgeMontage;

	/** 原地转身蒙太奇（Turn In Place，BP 的 TryPlayTurnInPlaceMontage 使用）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Move")
	TObjectPtr<UAnimMontage> TurnInPlaceMontage;

	/** 枢轴转身蒙太奇（Pivot，快速转身/方向切换用）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Move")
	TObjectPtr<UAnimMontage> PivotMontage;

	/** 装填/速装/抛弹/速抛蒙太奇（每把武器各自配置）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Move")
	TObjectPtr<UAnimMontage> LoadMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Move")
	TObjectPtr<UAnimMontage> FastLoadMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Move")
	TObjectPtr<UAnimMontage> TossMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Move")
	TObjectPtr<UAnimMontage> FastTossMontage;

	/** 防御性招式蒙太奇（HandleDefensive 统一使用；弹反播 L/R 段、防御播循环段、逆转逆装填播 ReReload 段，窗口由蒙太奇内 ANS 提供）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Move")
	TObjectPtr<UAnimMontage> DefensiveMontage;

	/** 逆转逆装填（ReverseReload 类型）播放的 DefensiveMontage 段名。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Move")
	FName ReReloadSectionName = TEXT("ReReload");

	/** 防御性招式类型：Parry=弹反（点按）/ Defend=防御（按住）/ ReverseReload=逆转逆装填（点按，需共鸣槽）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Move")
	ERHDefensiveType DefensiveType = ERHDefensiveType::Parry;

	/** 澶勫喅钂欏お濂囷細鐜╁鍜屾晫浜哄悇鎾竴娈碉紝鐜╁浣嶇疆鐢辨晫浜?AI 寮哄埗璋冩暣鍚庡啀鎾斁銆?*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Move")
	TObjectPtr<UAnimMontage> ExecutionMontage;

	/** 处决时敌人播放的攻击蒙太奇（从武器 DA 取，不再放敌人 DA）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Move")
	TObjectPtr<UAnimMontage> ExecutedMontage;

	// 处决时玩家被拉到的位置：敌人位置 + 敌人 forward * 此距离。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Move", meta = (ClampMin = "0"))
	float ExecutedDistance = 120.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Skill")
	TArray<TObjectPtr<URHOnomActionDefinition>> Skills;

	/** 终结技：本质是无消耗战技，轰鸣蓄满后可释放。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Skill")
	TObjectPtr<URHOnomActionDefinition> FinisherSkill;

	/** 切换技：切换到该武器后立刻使出的招式（Move Def，与普攻同源，无消耗）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Skill")
	TObjectPtr<URHMoveDefinition> SwitchTactic;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Move")
	TSoftObjectPtr<URHMoveDefinition> DefaultMoveDefinition;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Damage", meta = (ClampMin = "0"))
	float DefaultDamage = 9.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Damage", meta = (ClampMin = "0"))
	float DefaultResonanceDamage = 10.f;

	/** 受击蒙太奇 DA（按武器绑定；受击时由战斗组件按当前武器查询）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|HitReaction")
	TObjectPtr<UKnsHitReactionSettingsDataAsset> HitReaction;
};
