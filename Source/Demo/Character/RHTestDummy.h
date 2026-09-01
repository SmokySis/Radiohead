#pragma once

#include "CoreMinimal.h"
#include "Demo/Character/RHEnemyBase.h"
#include "Engine/TimerHandle.h"
#include "RHTestDummy.generated.h"

class UBoxComponent;
class UMaterialInterface;

/**
 * 木桩敌人：
 * - 默认不朝向玩家（构造时关闭 RHEnemyAIComponent 的自动旋转）。
 * - 可选受击判定框 ProbeBox：启用后按 ProbeCheckInterval 周期检测框内重叠的玩家，
 *   命中走完整受击管线（伤害/共振/韧性/受击动画，含玩家防御与无敌帧判定）。
 */
UCLASS()
class DEMO_API ARHTestDummy : public ARHEnemyBase
{
	GENERATED_BODY()

public:
	ARHTestDummy();

	virtual void BeginPlay() override;

	/** 受击判定框：位置/大小在细节面板调整；启用后按间隔检测框内玩家。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RH|TestDummy")
	TObjectPtr<UBoxComponent> ProbeBox;

	/** 判定框总开关：勾选 = BeginPlay 时启用；运行中可用 SetProbeEnabled 切换（同步 overlay 材质与检测定时器）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RH|TestDummy")
	bool bProbeEnabled = false;

	/** 检测间隔（秒）：每次检测时框内有玩家就结算一次受击。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RH|TestDummy", meta = (ClampMin = "0.05"))
	float ProbeCheckInterval = 1.f;

	/** 判定命中玩家的伤害。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RH|TestDummy|Hit", meta = (ClampMin = "0"))
	float ProbeDamage = 10.f;

	/** 判定命中玩家的共振伤害（玩家侧只走资源扣减）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RH|TestDummy|Hit", meta = (ClampMin = "0"))
	float ProbeResonanceDamage = 0.f;

	/** 判定命中玩家的韧性等级（打断判定；0 = 只掉血不播受击动画）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RH|TestDummy|Hit", meta = (ClampMin = "0"))
	int32 ProbePoiseLevel = 1;

	/** 判定框启用时给木桩 mesh 上的 overlay 材质（关闭时自动清除）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RH|TestDummy|Visual")
	TObjectPtr<UMaterialInterface> ProbeActiveOverlayMaterial;

	/** 运行期是否显示判定框线框（摆位/调试用）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RH|TestDummy|Visual")
	bool bShowProbeBoxInGame = true;

	/** 运行时切换判定框（同步 overlay 材质与检测定时器）。 */
	UFUNCTION(BlueprintCallable, Category = "RH|TestDummy")
	void SetProbeEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "RH|TestDummy")
	bool IsProbeEnabled() const { return bProbeEnabled; }

protected:
	/** 周期回调：查询框内重叠的玩家并结算受击。 */
	void TickProbe();

	/** 按 bProbeEnabled 同步碰撞/材质/定时器。 */
	void ApplyProbeEnabledState();

	void UpdateProbeBoxVisual();

	UPROPERTY(Transient)
	FTimerHandle ProbeTimerHandle;
};
