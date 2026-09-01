#pragma once

#include "CoreMinimal.h"

class AAIController;
class ARHEnemyBase;
class URHEnemyAIComponent;
class URHEnemyCombatComponent;

/** 敌人 AI 通用工具：StateTree 任务/条件/evaluator 共用（对齐参考项目 SKEnemyAI 风格）。 */
namespace RHEnemyAI
{
	/** 从 AIController 拿到它控制的敌人 Pawn（StateTree 上下文绑定 AIController 后直接可用）。 */
	DEMO_API ARHEnemyBase* GetEnemyFromController(const AAIController* Controller);

	DEMO_API URHEnemyAIComponent* GetAIComponent(const AAIController* Controller);

	DEMO_API URHEnemyCombatComponent* GetCombatComponent(const AAIController* Controller);
}
