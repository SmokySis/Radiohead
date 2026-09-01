#include "RHOnomComponent.h"

#include "Demo/Onom/RHCoreDefinition.h"
#include "Demo/Onom/RHOnomGainFeedbackDefinition.h"
#include "Demo/Onom/RHOnomSettings.h"
#include "Demo/Onom/RHWeaponDefinition.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

URHOnomComponent::URHOnomComponent()
{
	Slots.SetNum(3);
	for (ERHOnomValue& Slot : Slots)
	{
		Slot = ERHOnomValue::None;
	}
}

void URHOnomComponent::BeginPlay()
{
	Super::BeginPlay();

	const int32 SlotCount = FMath::Max(GetSlotCount(), 1);
	Slots.SetNum(SlotCount);
	for (ERHOnomValue& Slot : Slots)
	{
		Slot = ERHOnomValue::None;
	}

	BroadcastState();
}

int32 URHOnomComponent::AddOnom(const FRHOnomSourceRule& Rule, AActor* Instigator)
{
	if (!Rule.bEnabled || Rule.Count <= 0 || Rule.Type == ERHOnomValue::None)
	{
		return 0;
	}

	if (Rule.Mode == ERHOnomRuleMode::Remove)
	{
		return RemoveOnom(Rule.Count);
	}

	if (Rule.Type == ERHOnomValue::Broken)
	{
		int32 Added = 0;
		for (int32 i = 0; i < Rule.Count; ++i)
		{
			const ERHOnomGuardOutcome Outcome = AddBrokenOnom(Instigator);
			if (Outcome == ERHOnomGuardOutcome::None)
			{
				break;
			}
			++Added;
			if (Outcome == ERHOnomGuardOutcome::BigBreak)
			{
				break;
			}
		}
		return Added;
	}

	int32 Added = 0;
	while (Added < Rule.Count)
	{
		const int32 EmptyIndex = Slots.IndexOfByKey(ERHOnomValue::None);
		if (EmptyIndex == INDEX_NONE)
		{
			break;
		}

		Slots[EmptyIndex] = Rule.Type;
		++Added;
	}

	if (Added > 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("RH Onom Added: %s x%d"), *UEnum::GetValueAsString(Rule.Type), Added);
		FRHOnomEventData Event;
		Event.Type = Rule.Type;
		Event.Count = Added;
		Event.Instigator = Instigator;
		Event.State = GetOnomState();
		OnOnomAdded.Broadcast(Event);
		BroadcastState();
		PlayGainSound(Rule.Type);
	}

	return Added;
}

ERHOnomGuardOutcome URHOnomComponent::AddBrokenOnom(AActor* Instigator)
{
	return AddBrokenOnomInternal(Instigator, true);
}

ERHOnomGuardOutcome URHOnomComponent::AddBrokenOnomInternal(AActor* Instigator, bool bBroadcast)
{
	// 灰到 3 后下一次防御受击破防（阈值由玩法定死为 3，配置已从 Settings 移除）。
	const int32 GreyBreakCount = 3;

	// 以“本次受击前”的灰数为准：灰到 3 后不会立刻破防，下一次防御受击（第 4 次）才触发。
	int32 GreyCountBefore = 0;
	for (const ERHOnomValue& Slot : Slots)
	{
		if (Slot == ERHOnomValue::Broken)
		{
			++GreyCountBefore;
		}
	}

	ERHOnomGuardOutcome Outcome = ERHOnomGuardOutcome::Added;
	if (GreyCountBefore >= GreyBreakCount)
	{
		// 大破防：清空灰色、失去共鸣、不扣轰鸣；本次不再新增灰。
		for (ERHOnomValue& Slot : Slots)
		{
			if (Slot == ERHOnomValue::Broken)
			{
				Slot = ERHOnomValue::None;
			}
		}
		ClearResonance(false);

		if (bBroadcast)
		{
			FRHOnomBigBreakData Data;
			Data.FixedDamage = 0.f; // 破防不扣血（算玩家成功防下）。
			Data.StunSeconds = 2.f;
			OnBigBreakTriggered.Broadcast(Data);
		}
		Outcome = ERHOnomGuardOutcome::BigBreak;
	}
	else
	{
		const int32 EmptyIndex = Slots.IndexOfByKey(ERHOnomValue::None);
		if (EmptyIndex == INDEX_NONE)
		{
			// 槽满：灰色吞掉 1 个现有非灰 Onom（破碎不吞破碎；平调同为非灰，可被吞）。
			int32 SwallowIndex = INDEX_NONE;
			for (int32 i = Slots.Num() - 1; i >= 0; --i)
			{
				if (Slots[i] == ERHOnomValue::Positive || Slots[i] == ERHOnomValue::Negative || Slots[i] == ERHOnomValue::Neutral)
				{
					SwallowIndex = i;
					break;
				}
			}

			if (SwallowIndex == INDEX_NONE)
			{
				// 全灰且未满阈值（阈值应 ≤ 槽数），兜底直接拒绝。
				return ERHOnomGuardOutcome::None;
			}

			Slots[SwallowIndex] = ERHOnomValue::Broken;
			Outcome = ERHOnomGuardOutcome::Swallowed;
		}
		else
		{
			Slots[EmptyIndex] = ERHOnomValue::Broken;
		}
	}

	if (bBroadcast)
	{
		UE_LOG(LogTemp, Warning, TEXT("RH Broken Onom Outcome: %s"), *UEnum::GetValueAsString(Outcome));
		FRHOnomEventData Event;
		Event.Type = ERHOnomValue::Broken;
		Event.Count = 1;
		Event.Instigator = Instigator;
		Event.State = GetOnomState();
		OnOnomAdded.Broadcast(Event);
		BroadcastState();
		if (Outcome == ERHOnomGuardOutcome::BigBreak)
		{
			// 大破防音效由受击蒙太奇上的官方 Play Sound AN 负责，这里只回落破碎音形音效。
			PlayGainSound(ERHOnomValue::Broken);
		}
		else
		{
			PlayGainSound(ERHOnomValue::Broken);
		}
	}

	return Outcome;
}

int32 URHOnomComponent::RemoveOnom(int32 Count)
{
	if (Count <= 0)
	{
		return 0;
	}

	int32 Removed = 0;

	// 优先移除非灰（破碎不参与消耗数量计算），实在没有非灰才移破碎。
	for (int32 Pass = 0; Pass < 2 && Removed < Count; ++Pass)
	{
		const bool bNonGreyPass = Pass == 0;
		for (int32 Index = Slots.Num() - 1; Index >= 0 && Removed < Count; --Index)
		{
			const bool bIsNonGrey = Slots[Index] == ERHOnomValue::Positive || Slots[Index] == ERHOnomValue::Negative || Slots[Index] == ERHOnomValue::Neutral;
			const bool bIsBroken = Slots[Index] == ERHOnomValue::Broken;
			if ((bNonGreyPass && bIsNonGrey) || (!bNonGreyPass && bIsBroken))
			{
				Slots[Index] = ERHOnomValue::None;
				++Removed;
			}
		}
	}

	if (Removed > 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("RH Onom Removed: x%d"), Removed);
		FRHOnomEventData Event;
		Event.Type = ERHOnomValue::None;
		Event.Count = Removed;
		Event.State = GetOnomState();
		OnOnomRemoved.Broadcast(Event);
		BroadcastState();
	}

	return Removed;
}

int32 URHOnomComponent::ConvertGreyOnom(ERHOnomValue Value)
{
	// 灰→灰无意义；None 视为无效目标，均不处理。
	if (Value == ERHOnomValue::None || Value == ERHOnomValue::Broken)
	{
		return 0;
	}

	int32 Converted = 0;
	for (ERHOnomValue& Slot : Slots)
	{
		if (Slot == ERHOnomValue::Broken)
		{
			Slot = Value;
			++Converted;
		}
	}

	if (Converted > 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("RH Onom Grey Converted: x%d -> %s"),
			Converted, *UEnum::GetValueAsString(Value));
		BroadcastState();
	}
	return Converted;
}

bool URHOnomComponent::TryStoreToResonance()
{
	if (HasGreyOnom())
	{
		return false;
	}

	const int32 NonGrey = GetNonGreyOnomCount();
	if (NonGrey < 1)
	{
		return false;
	}

	const int32 OldLayers = ResonanceLayers;
	const int32 NewLevel = FMath::Clamp(NonGrey, 1, GetMaxLayers());
	const int32 Sum = GetOnomSignedSum();

	ResonanceLayers = NewLevel;
	ResonanceType = Sum > 0 ? ERHOnomPolarity::Major : (Sum < 0 ? ERHOnomPolarity::Minor : ERHOnomPolarity::Neutral);
	// 存储语义：共鸣槽记录装填时的手牌和值（大小由层数外的音形构成决定）。
	ResonanceSignedSum = Sum;
	ResonanceTimeRemaining = GetResonanceDecayForCurrentLayer();
	// 正常装填：衰减倍率回到 1.0（覆盖攻击攒共鸣的武器倍率）。
	ResonanceDecayRate = 1.f;
	StartResonanceTimer();

	for (ERHOnomValue& Slot : Slots)
	{
		Slot = ERHOnomValue::None;
	}

	FRHOnomResonanceEventData Event;
	Event.OldLayers = OldLayers;
	Event.NewLayers = ResonanceLayers;
	Event.Type = ResonanceType;
	Event.SignedSum = ResonanceSignedSum;
	Event.TimeRemaining = ResonanceTimeRemaining;

	OnResonanceStored.Broadcast(Event);
	OnStorageLevelChanged.Broadcast(OldLayers, ResonanceLayers);
	BroadcastState();
	PlayResonanceStoredSound(ResonanceType);
	UE_LOG(LogTemp, Warning, TEXT("RH Resonance Stored: level=%d type=%s sum=%d"),
		ResonanceLayers, *UEnum::GetValueAsString(ResonanceType), ResonanceSignedSum);
	return true;
}

int32 URHOnomComponent::ReloadFromResonance(float ConsumeSeconds, bool bInvertColor)
{
	// 按共鸣等级生成手牌音形：大调→正、小调→负、平调→平调（Neutral）；
	// bInvertColor=true 逆转：红→蓝、蓝→红、平→平。
	// 放置规则：不覆盖非灰，可覆盖灰，再填空位；放不下的丢弃。
	// 生成数量按消耗的共鸣时长比例折算（全量=1 倍层数），消耗时长默认远大于剩余时长=直接用完。
	if (ResonanceLayers <= 0 || ResonanceType == ERHOnomPolarity::None || ConsumeSeconds <= 0.f)
	{
		return 0;
	}

	ERHOnomValue Value;
	if (bInvertColor)
	{
		Value = ResonanceType == ERHOnomPolarity::Major ? ERHOnomValue::Negative
			: (ResonanceType == ERHOnomPolarity::Minor ? ERHOnomValue::Positive : ERHOnomValue::Neutral);
	}
	else
	{
		Value = ResonanceType == ERHOnomPolarity::Major ? ERHOnomValue::Positive
			: (ResonanceType == ERHOnomPolarity::Minor ? ERHOnomValue::Negative : ERHOnomValue::Neutral);
	}

	// 生成数 = 层数 × 消耗比例（四舍五入，至少 1）。
	const float Ratio = FMath::Clamp(ConsumeSeconds / FMath::Max(ResonanceTimeRemaining, KINDA_SMALL_NUMBER), 0.f, 1.f);
	const int32 Expected = FMath::Max(1, FMath::RoundToInt(ResonanceLayers * Ratio));
	const int32 Placed = PlaceOnomValue(Value, Expected);
	// 扣共鸣时长（归零则共鸣消失并广播）。
	SubtractResonanceTime(ConsumeSeconds);
	BroadcastState();
	if (Placed > 0)
	{
		PlayGainSound(Value);
	}
	UE_LOG(LogTemp, Warning, TEXT("RH Reload(Reverse Load): placed=%d value=%s consume=%.1fs"),
		Placed, *UEnum::GetValueAsString(Value), ConsumeSeconds);
	return Placed;
}

int32 URHOnomComponent::PlaceOnomValue(ERHOnomValue Value, int32 Count)
{
	if (Count <= 0)
	{
		return 0;
	}

	int32 ToPlace = Count;

	// 第一遍：覆盖灰色音形。
	for (ERHOnomValue& Slot : Slots)
	{
		if (ToPlace <= 0)
		{
			break;
		}
		if (Slot == ERHOnomValue::Broken)
		{
			Slot = Value;
			--ToPlace;
		}
	}

	// 第二遍：填空位。
	for (ERHOnomValue& Slot : Slots)
	{
		if (ToPlace <= 0)
		{
			break;
		}
		if (Slot == ERHOnomValue::None)
		{
			Slot = Value;
			--ToPlace;
		}
	}

	return Count - ToPlace;
}

bool URHOnomComponent::PrepareReverseReload(float ConsumeSeconds)
{
	// 无共鸣/类型无效/时长无效 → 返回 false，窗口不生效。
	if (ResonanceLayers <= 0 || ResonanceType == ERHOnomPolarity::None || ConsumeSeconds <= 0.f)
	{
		return false;
	}

	// 记录共鸣类型与待生成数量（与 ReloadFromResonance 同公式折算），随后立即消耗共鸣（沉没成本）。
	ReverseReloadType = ResonanceType;
	const float Ratio = FMath::Clamp(ConsumeSeconds / FMath::Max(ResonanceTimeRemaining, KINDA_SMALL_NUMBER), 0.f, 1.f);
	ReverseReloadPendingCount = FMath::Max(1, FMath::RoundToInt(ResonanceLayers * Ratio));
	SubtractResonanceTime(ConsumeSeconds);
	BroadcastState();
	UE_LOG(LogTemp, Warning, TEXT("RH Reverse Reload: prepared type=%s pending=%d consume=%.1fs"),
		*UEnum::GetValueAsString(ReverseReloadType), ReverseReloadPendingCount, ConsumeSeconds);
	return true;
}

int32 URHOnomComponent::DeliverReverseReload()
{
	if (ReverseReloadPendingCount <= 0 || ReverseReloadType == ERHOnomPolarity::None)
	{
		return 0;
	}

	// 反色生成：红→蓝、蓝→红、平→平。
	const ERHOnomValue Value = ReverseReloadType == ERHOnomPolarity::Major ? ERHOnomValue::Negative
		: (ReverseReloadType == ERHOnomPolarity::Minor ? ERHOnomValue::Positive : ERHOnomValue::Neutral);

	const int32 Placed = PlaceOnomValue(Value, ReverseReloadPendingCount);
	ReverseReloadPendingCount = 0;
	ReverseReloadType = ERHOnomPolarity::None;
	BroadcastState();
	if (Placed > 0)
	{
		PlayGainSound(Value);
	}
	UE_LOG(LogTemp, Warning, TEXT("RH Reverse Reload: delivered placed=%d value=%s"),
		Placed, *UEnum::GetValueAsString(Value));
	return Placed;
}

void URHOnomComponent::AddResonanceLayer(ERHOnomPolarity Type, float DecayRate)
{
	if (Type == ERHOnomPolarity::None)
	{
		return;
	}

	ResonanceLayers = FMath::Clamp(ResonanceLayers + 1, 1, GetMaxLayers());
	ResonanceType = Type;
	// 存储语义：与装填一致记录当前和值大小（攻击攒的按层数近似）。
	ResonanceSignedSum = Type == ERHOnomPolarity::Major ? ResonanceLayers
		: (Type == ERHOnomPolarity::Minor ? -ResonanceLayers : 0);
	ResonanceTimeRemaining = GetResonanceDecayForCurrentLayer();
	// 攻击攒的共鸣衰减速率由调用方按当前武器 DA 的 ResonanceDecayRate 传入（默认 1.0）；装填/相杀/切武器仍重置回 1.0。
	ResonanceDecayRate = FMath::Max(DecayRate, 0.f);
	StartResonanceTimer();

	FRHOnomResonanceEventData Event;
	Event.OldLayers = ResonanceLayers - 1;
	Event.NewLayers = ResonanceLayers;
	Event.Type = ResonanceType;
	Event.SignedSum = ResonanceSignedSum;
	Event.TimeRemaining = ResonanceTimeRemaining;
	OnResonanceStored.Broadcast(Event);
	OnStorageLevelChanged.Broadcast(Event.OldLayers, ResonanceLayers);
	BroadcastState();
	UE_LOG(LogTemp, Warning, TEXT("RH Resonance Gained by Hit: level=%d type=%s (decay x%.1f)"),
		ResonanceLayers, *UEnum::GetValueAsString(ResonanceType), ResonanceDecayRate);
}

void URHOnomComponent::SetResonanceDecayRate(float InRate)
{
	ResonanceDecayRate = FMath::Max(InRate, 0.f);
}

bool URHOnomComponent::SetResonance(ERHOnomPolarity Type, int32 Level)
{
	if (Type == ERHOnomPolarity::None || Type == ERHOnomPolarity::Broken)
	{
		return false;
	}

	const int32 OldLayers = ResonanceLayers;
	const int32 NewLevel = FMath::Clamp(Level, 1, GetMaxLayers());

	ResonanceLayers = NewLevel;
	ResonanceType = Type;
	// 存储语义：与攻击攒一致按层数近似和值。
	ResonanceSignedSum = Type == ERHOnomPolarity::Major ? NewLevel
		: (Type == ERHOnomPolarity::Minor ? -NewLevel : 0);
	ResonanceTimeRemaining = GetResonanceDecayForCurrentLayer();
	// 奖励性获得（相杀等）：衰减回 1.0，同装填，不吃攻击攒的武器倍率。
	ResonanceDecayRate = 1.f;
	StartResonanceTimer();

	FRHOnomResonanceEventData Event;
	Event.OldLayers = OldLayers;
	Event.NewLayers = ResonanceLayers;
	Event.Type = ResonanceType;
	Event.SignedSum = ResonanceSignedSum;
	Event.TimeRemaining = ResonanceTimeRemaining;
	OnResonanceStored.Broadcast(Event);
	OnStorageLevelChanged.Broadcast(OldLayers, ResonanceLayers);
	BroadcastState();
	PlayResonanceStoredSound(ResonanceType);
	UE_LOG(LogTemp, Warning, TEXT("RH Resonance Set (Override): level=%d type=%s sum=%d"),
		ResonanceLayers, *UEnum::GetValueAsString(ResonanceType), ResonanceSignedSum);
	return true;
}

ERHOnomPolarity URHOnomComponent::GetPolarityFromValue(ERHOnomValue Value)
{
	switch (Value)
	{
	case ERHOnomValue::Positive: return ERHOnomPolarity::Major;
	case ERHOnomValue::Negative: return ERHOnomPolarity::Minor;
	case ERHOnomValue::Neutral: return ERHOnomPolarity::Neutral;
	case ERHOnomValue::Broken: return ERHOnomPolarity::Broken;
	default: return ERHOnomPolarity::None;
	}
}

FRHOnomConsumptionData URHOnomComponent::ConsumeOnom(int32 Amount)
{
	FRHOnomConsumptionData Data;
	if (Amount <= 0)
	{
		return Data;
	}

	const int32 NonGrey = GetNonGreyOnomCount();
	const int32 Available = NonGrey + (ResonanceLayers > 0 ? 1 : 0);
	if (Available < Amount)
	{
		return Data;
	}

	// 只累计“实际被消耗掉”的音形和值；未消耗的保留牌不参与本次动作的极性/变体计算。
	int32 ConsumedSum = 0;
	int32 Remaining = Amount;

	if (ResonanceLayers > 0)
	{
		// 优先扣共鸣：固定算 1 格。参与和值计算时只算一个音形（按极性 ±1/0），等级只影响增幅与充能。
		Data.bResonanceConsumed = true;
		Data.ResonanceLevel = ResonanceLayers;
		Data.ResonanceType = ResonanceType;
		ConsumedSum += ResonanceType == ERHOnomPolarity::Major ? 1
			: (ResonanceType == ERHOnomPolarity::Minor ? -1 : 0);
		ClearResonance(false);
		--Remaining;
	}

	for (int32 Index = Slots.Num() - 1; Index >= 0 && Remaining > 0; --Index)
	{
		// 红/蓝/平调都可消耗：红=+1、蓝=-1、平调=0（占 1 格但不贡献和值）。
		if (Slots[Index] == ERHOnomValue::Positive || Slots[Index] == ERHOnomValue::Negative || Slots[Index] == ERHOnomValue::Neutral)
		{
			ConsumedSum += (Slots[Index] == ERHOnomValue::Positive) ? 1
				: (Slots[Index] == ERHOnomValue::Negative ? -1 : 0);
			Slots[Index] = ERHOnomValue::None;
			--Remaining;
			++Data.HandConsumed;
		}
	}

	// 释放技能时灰色一起清空。
	for (ERHOnomValue& Slot : Slots)
	{
		if (Slot == ERHOnomValue::Broken)
		{
			Slot = ERHOnomValue::None;
		}
	}

	Data.SignedSum = ConsumedSum;
	Data.ConsumedCount = Data.HandConsumed + (Data.bResonanceConsumed ? 1 : 0);
	Data.AbsoluteSum = FMath::Abs(Data.SignedSum);

	FRHOnomEventData Event;
	Event.Type = ERHOnomValue::None;
	Event.Count = Data.ConsumedCount;
	Event.State = GetOnomState();
	OnOnomRemoved.Broadcast(Event);
	BroadcastState();

	UE_LOG(LogTemp, Warning, TEXT("RH Onom Consumed: amount=%d hand=%d resonance=%d sum=%d"),
		Data.ConsumedCount, Data.HandConsumed, Data.ResonanceLevel, Data.SignedSum);
	return Data;
}

FRHOnomConsumptionData URHOnomComponent::SimulateConsumeOnom(int32 Amount) const
{
	FRHOnomConsumptionData Data;
	if (Amount <= 0)
	{
		return Data;
	}

	// 与 ConsumeOnom 完全同序/同折算，但不改任何状态（纯模拟，供释放前匹配用）。
	const int32 NonGrey = GetNonGreyOnomCount();
	const int32 Available = NonGrey + (ResonanceLayers > 0 ? 1 : 0);
	if (Available < Amount)
	{
		return Data;
	}

	int32 ConsumedSum = 0;
	int32 Remaining = Amount;

	if (ResonanceLayers > 0)
	{
		// 优先扣共鸣：固定算 1 格，按极性折算 ±1/0。
		Data.bResonanceConsumed = true;
		Data.ResonanceLevel = ResonanceLayers;
		Data.ResonanceType = ResonanceType;
		ConsumedSum += ResonanceType == ERHOnomPolarity::Major ? 1
			: (ResonanceType == ERHOnomPolarity::Minor ? -1 : 0);
		--Remaining;
	}

	for (int32 Index = Slots.Num() - 1; Index >= 0 && Remaining > 0; --Index)
	{
		// 红/蓝/平调都可消耗：红=+1、蓝=-1、平调=0。
		if (Slots[Index] == ERHOnomValue::Positive || Slots[Index] == ERHOnomValue::Negative || Slots[Index] == ERHOnomValue::Neutral)
		{
			ConsumedSum += (Slots[Index] == ERHOnomValue::Positive) ? 1
				: (Slots[Index] == ERHOnomValue::Negative ? -1 : 0);
			--Remaining;
			++Data.HandConsumed;
		}
	}

	Data.SignedSum = ConsumedSum;
	Data.ConsumedCount = Data.HandConsumed + (Data.bResonanceConsumed ? 1 : 0);
	Data.AbsoluteSum = FMath::Abs(Data.SignedSum);
	return Data;
}

bool URHOnomComponent::TryConsumeHandOnom(int32 Amount, FRHOnomConsumptionData& OutData)
{
	OutData = FRHOnomConsumptionData();
	if (Amount <= 0 || GetNonGreyOnomCount() < Amount)
	{
		return false;
	}

	// 只累计实际消耗掉的手牌和值（红/蓝/平调都可消耗：+1/-1/0）。
	int32 ConsumedSum = 0;
	for (int32 Index = Slots.Num() - 1; Index >= 0 && OutData.HandConsumed < Amount; --Index)
	{
		if (Slots[Index] == ERHOnomValue::Positive || Slots[Index] == ERHOnomValue::Negative || Slots[Index] == ERHOnomValue::Neutral)
		{
			ConsumedSum += (Slots[Index] == ERHOnomValue::Positive) ? 1
				: (Slots[Index] == ERHOnomValue::Negative ? -1 : 0);
			Slots[Index] = ERHOnomValue::None;
			++OutData.HandConsumed;
		}
	}

	for (ERHOnomValue& Slot : Slots)
	{
		if (Slot == ERHOnomValue::Broken)
		{
			Slot = ERHOnomValue::None;
		}
	}

	OutData.SignedSum = ConsumedSum;
	OutData.ConsumedCount = OutData.HandConsumed;
	OutData.AbsoluteSum = FMath::Abs(OutData.SignedSum);

	FRHOnomEventData Event;
	Event.Type = ERHOnomValue::None;
	Event.Count = OutData.ConsumedCount;
	Event.State = GetOnomState();
	OnOnomRemoved.Broadcast(Event);
	BroadcastState();
	return true;
}

FRHOnomConsumptionData URHOnomComponent::ClearHandOnom()
{
	FRHOnomConsumptionData Data;
	Data.HandConsumed = GetNonGreyOnomCount();
	Data.ConsumedCount = Data.HandConsumed;
	Data.SignedSum = GetOnomSignedSum();
	Data.AbsoluteSum = FMath::Abs(Data.SignedSum);
	Data.ResonanceLevel = ResonanceLayers;
	Data.ResonanceType = ResonanceType;

	for (ERHOnomValue& Slot : Slots)
	{
		Slot = ERHOnomValue::None;
	}

	FRHOnomEventData Event;
	Event.Type = ERHOnomValue::None;
	Event.Count = Data.ConsumedCount;
	Event.State = GetOnomState();
	OnOnomCleared.Broadcast(Event);
	BroadcastState();
	return Data;
}

FRHOnomConsumptionData URHOnomComponent::ConsumeAllOnom()
{
	FRHOnomConsumptionData Data = GetConsumptionData();

	for (ERHOnomValue& Slot : Slots)
	{
		Slot = ERHOnomValue::None;
	}

	ClearResonance(false);

	FRHOnomEventData Event;
	Event.Type = ERHOnomValue::None;
	Event.Count = Data.ConsumedCount;
	Event.State = GetOnomState();
	OnOnomCleared.Broadcast(Event);
	BroadcastState();

	UE_LOG(LogTemp, Warning, TEXT("RH Onom Cleared All: count=%d sum=%d"), Data.ConsumedCount, Data.SignedSum);
	return Data;
}

FRHOnomConsumptionData URHOnomComponent::GetConsumptionData() const
{
	FRHOnomConsumptionData Data;
	const int32 NonGrey = GetNonGreyOnomCount();
	Data.HandConsumed = NonGrey;
	Data.ResonanceLevel = ResonanceLayers;
	Data.ResonanceType = ResonanceType;
	Data.bResonanceConsumed = ResonanceLayers > 0;
	Data.ConsumedCount = NonGrey + (ResonanceLayers > 0 ? 1 : 0);
	// 共鸣参与和值/绝对值计算时只算一个音形（按极性 ±1/0），等级只影响增幅与充能。
	Data.SignedSum = GetOnomSignedSum() + (ResonanceLayers > 0
		? (ResonanceType == ERHOnomPolarity::Major ? 1 : (ResonanceType == ERHOnomPolarity::Minor ? -1 : 0))
		: 0);
	Data.AbsoluteSum = FMath::Abs(Data.SignedSum);
	return Data;
}

FRHOnomConsumptionData URHOnomComponent::GetHandConsumptionData() const
{
	FRHOnomConsumptionData Data;
	Data.HandConsumed = GetNonGreyOnomCount();
	Data.ConsumedCount = Data.HandConsumed;
	Data.SignedSum = GetOnomSignedSum();
	Data.AbsoluteSum = FMath::Abs(Data.SignedSum);
	return Data;
}

bool URHOnomComponent::CanConsumeOnom(int32 Amount) const
{
	return Amount <= 0 || GetConsumptionData().ConsumedCount >= Amount;
}

bool URHOnomComponent::TryConsumeOnom(int32 Amount)
{
	if (!CanConsumeOnom(Amount))
	{
		return false;
	}

	ConsumeOnom(Amount);
	return true;
}

FRHOnomAmplification URHOnomComponent::ComputeAmplification(const FRHOnomConsumptionData& Consumption, const URHWeaponDefinition* Weapon) const
{
	FRHOnomAmplification Result;
	if (!Consumption.bResonanceConsumed || Consumption.ResonanceLevel <= 0)
	{
		return Result;
	}

	const float Factor = Settings ? Settings->GetResonanceLevelFactor(Consumption.ResonanceLevel) : 1.f;
	const bool bMatch = Weapon && Weapon->PreferredPolarity != ERHOnomPolarity::None && Weapon->PreferredPolarity == Consumption.ResonanceType;

	switch (Consumption.ResonanceType)
	{
	case ERHOnomPolarity::Major:
		Result.DamageMultiplier = Factor * (bMatch ? Weapon->DamageCoeff : 1.f);
		if (bMatch)
		{
			Result.ResonanceDamageMultiplier = Weapon->ResonanceDamageCoeff;
			Result.ChargeMultiplier = Weapon->ChargeCoeff;
		}
		break;
	case ERHOnomPolarity::Minor:
		Result.ResonanceDamageMultiplier = Factor * (bMatch ? Weapon->ResonanceDamageCoeff : 1.f);
		if (bMatch)
		{
			Result.DamageMultiplier = Weapon->DamageCoeff;
			Result.ChargeMultiplier = Weapon->ChargeCoeff;
		}
		break;
	case ERHOnomPolarity::Neutral:
		Result.ChargeMultiplier = Factor * (bMatch ? Weapon->ChargeCoeff : 1.f);
		if (bMatch)
		{
			Result.DamageMultiplier = Weapon->DamageCoeff;
			Result.ResonanceDamageMultiplier = Weapon->ResonanceDamageCoeff;
		}
		break;
	default:
		break;
	}

	UE_LOG(LogTemp, Warning, TEXT("RH Amplify: type=%s level=%d factor=%.2f match=%d (weapon=%s) -> dmg=%.2f resDmg=%.2f charge=%.2f"),
		*UEnum::GetValueAsString(Consumption.ResonanceType), Consumption.ResonanceLevel, Factor, bMatch ? 1 : 0,
		Weapon ? *Weapon->GetName() : TEXT("None"),
		Result.DamageMultiplier, Result.ResonanceDamageMultiplier, Result.ChargeMultiplier);

	return Result;
}

void URHOnomComponent::ApplyDamageTakenRule(AActor* Instigator)
{
	bool bClearedAll = true;

	// 音律核心：可把受伤改为"按规则失去/增加"，默认仍是清空全部。
	if (CoreDefinition && CoreDefinition->DamageTakenMode == ERHOnomDamageTakenMode::ApplyRule)
	{
		const FRHOnomSourceRule& Rule = CoreDefinition->DamageTakenRule;
		if (Rule.bEnabled && Rule.Count > 0)
		{
			if (Rule.Mode == ERHOnomRuleMode::Add)
			{
				AddOnom(Rule, Instigator);
			}
			else
			{
				RemoveOnom(Rule.Count);
			}
			bClearedAll = false;
		}
	}

	if (bClearedAll)
	{
		int32 Cleared = 0;
		for (ERHOnomValue& Slot : Slots)
		{
			if (Slot != ERHOnomValue::None)
			{
				Slot = ERHOnomValue::None;
				++Cleared;
			}
		}

		FRHOnomEventData Event;
		Event.Type = ERHOnomValue::None;
		Event.Count = Cleared;
		Event.Instigator = Instigator;
		Event.State = GetOnomState();
		OnOnomCleared.Broadcast(Event);
		BroadcastState();
	}

	// 受击共鸣扣时（数值已由 Core DA 的 DamageTakenEffects 接管，这里保留旧回落用固定值）。
	const float Penalty = 2.f;
	SubtractResonanceTime(Penalty);
	UE_LOG(LogTemp, Warning, TEXT("RH Damage Taken: mode=%s, resonance -%.1fs (charge untouched)"),
		CoreDefinition && CoreDefinition->DamageTakenMode == ERHOnomDamageTakenMode::ApplyRule ? TEXT("Rule") : TEXT("ClearAll"), Penalty);
}

bool URHOnomComponent::HasGreyOnom() const
{
	return Slots.Contains(ERHOnomValue::Broken);
}

int32 URHOnomComponent::GetNonGreyOnomCount() const
{
	int32 Count = 0;
	for (const ERHOnomValue& Slot : Slots)
	{
		if (Slot == ERHOnomValue::Positive || Slot == ERHOnomValue::Negative || Slot == ERHOnomValue::Neutral)
		{
			++Count;
		}
	}
	return Count;
}

int32 URHOnomComponent::GetOnomSignedSum() const
{
	int32 Sum = 0;
	for (const ERHOnomValue& Slot : Slots)
	{
		if (Slot == ERHOnomValue::Positive)
		{
			Sum += 1;
		}
		else if (Slot == ERHOnomValue::Negative)
		{
			Sum -= 1;
		}
	}
	return Sum;
}

int32 URHOnomComponent::GetResonanceLayers() const
{
	return ResonanceLayers;
}

ERHOnomPolarity URHOnomComponent::GetResonanceType() const
{
	return ResonanceType;
}

int32 URHOnomComponent::GetResonanceSignedSum() const
{
	return ResonanceSignedSum;
}

int32 URHOnomComponent::GetTotalConsumableOnom() const
{
	return GetNonGreyOnomCount() + (ResonanceLayers > 0 ? 1 : 0);
}

float URHOnomComponent::GetChargePercent() const
{
	return ChargePercent;
}

void URHOnomComponent::AddCharge(float Percent)
{
	if (Percent <= 0.f)
	{
		return;
	}

	const float OldPercent = ChargePercent;
	const float MaxPercent = GetChargeMaxPercent();
	ChargePercent = FMath::Clamp(ChargePercent + Percent, 0.f, MaxPercent);

	if (FMath::IsNearlyEqual(OldPercent, ChargePercent))
	{
		return;
	}

	OnChargeChanged.Broadcast(ChargePercent);
	if (OldPercent < MaxPercent && ChargePercent >= MaxPercent)
	{
		OnChargeFull.Broadcast();
	}
	BroadcastState();
}

void URHOnomComponent::ResetCharge()
{
	if (FMath::IsNearlyZero(ChargePercent))
	{
		return;
	}

	ChargePercent = 0.f;
	OnChargeChanged.Broadcast(ChargePercent);
	BroadcastState();
}

float URHOnomComponent::GetChargeMaxPercent() const
{
	return Settings ? FMath::Max(Settings->ChargeMaxPercent, 0.f) : 100.f;
}

float URHOnomComponent::GetResonanceDecaySecondsForLevel(int32 Level) const
{
	if (Settings)
	{
		return Settings->GetResonanceDecayForLayer(Level);
	}

	const float Defaults[] = { 10.f, 8.f, 6.f };
	if (Level >= 1 && Level <= 3)
	{
		return Defaults[Level - 1];
	}
	return 10.f;
}

void URHOnomComponent::AddResonanceTime(float Seconds)
{
	if (ResonanceLayers <= 0 || Seconds <= 0.f)
	{
		return;
	}

	ResonanceTimeRemaining += Seconds;
	StartResonanceTimer();

	FRHOnomResonanceEventData Event;
	Event.OldLayers = ResonanceLayers;
	Event.NewLayers = ResonanceLayers;
	Event.Type = ResonanceType;
	Event.SignedSum = ResonanceSignedSum;
	Event.TimeRemaining = ResonanceTimeRemaining;
	OnResonanceTimeChanged.Broadcast(Event);
	BroadcastState();
}

void URHOnomComponent::SubtractResonanceTime(float Seconds)
{
	if (ResonanceLayers <= 0 || Seconds <= 0.f)
	{
		return;
	}

	ResonanceTimeRemaining -= Seconds;
	if (ResonanceTimeRemaining <= 0.f)
	{
		ClearResonance(true);
		return;
	}
	StartResonanceTimer();

	FRHOnomResonanceEventData Event;
	Event.OldLayers = ResonanceLayers;
	Event.NewLayers = ResonanceLayers;
	Event.Type = ResonanceType;
	Event.SignedSum = ResonanceSignedSum;
	Event.TimeRemaining = ResonanceTimeRemaining;
	OnResonanceTimeChanged.Broadcast(Event);
	BroadcastState();
}

FRHOnomState URHOnomComponent::GetOnomState() const
{
	FRHOnomState State;
	State.Slots = Slots;
	State.ResonanceLayers = ResonanceLayers;
	State.ResonanceType = ResonanceType;
	State.ResonanceSignedSum = ResonanceSignedSum;
	State.ResonanceTimeRemaining = ResonanceTimeRemaining;
	State.ChargePercent = ChargePercent;
	return State;
}

void URHOnomComponent::PlayGainSound(ERHOnomValue Type)
{
	if (!GainFeedbackDefinition || !GetWorld())
	{
		return;
	}

	ERHOnomPolarity Polarity = GetPolarityFromValue(Type);
	if (Polarity == ERHOnomPolarity::None)
	{
		return;
	}

	FRHOnomGainFeedbackEntry Entry;
	if (GainFeedbackDefinition->GetEntry(Polarity, Entry) && Entry.Sound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), Entry.Sound, GetOwner()->GetActorLocation());
	}
}

void URHOnomComponent::PlayResonanceStoredSound(ERHOnomPolarity InResonanceType)
{
	if (!GainFeedbackDefinition || !GetWorld() || InResonanceType == ERHOnomPolarity::None)
	{
		return;
	}

	FRHOnomGainFeedbackEntry Entry;
	if (GainFeedbackDefinition->GetEntry(InResonanceType, Entry) && Entry.Sound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), Entry.Sound, GetOwner()->GetActorLocation());
	}
}

int32 URHOnomComponent::GetSlotCount() const
{
	return Settings ? FMath::Max(Settings->SlotCount, 1) : 3;
}

int32 URHOnomComponent::GetMaxLayers() const
{
	return Settings ? FMath::Max(Settings->MaxResonanceLayers, 0) : 3;
}

float URHOnomComponent::GetResonanceDecayForCurrentLayer() const
{
	if (Settings)
	{
		return Settings->GetResonanceDecayForLayer(ResonanceLayers);
	}
	return 10.f;
}

void URHOnomComponent::ClearResonance(bool bBroadcastExpired)
{
	const bool bHadResonance = ResonanceLayers > 0;
	StopResonanceTimer();
	ResonanceLayers = 0;
	ResonanceType = ERHOnomPolarity::None;
	ResonanceSignedSum = 0;
	ResonanceTimeRemaining = 0.f;
	ResonanceDecayRate = 1.f;

	if (bBroadcastExpired && bHadResonance)
	{
		OnResonanceExpired.Broadcast();
		UE_LOG(LogTemp, Warning, TEXT("RH Resonance Expired"));
	}
	BroadcastState();
}

void URHOnomComponent::StartResonanceTimer()
{
	if (ResonanceLayers <= 0 || !GetWorld())
	{
		StopResonanceTimer();
		return;
	}

	GetWorld()->GetTimerManager().SetTimer(
		ResonanceTimerHandle,
		FTimerDelegate::CreateUObject(this, &URHOnomComponent::HandleResonanceTimerElapsed),
		0.1f,
		true);
}

void URHOnomComponent::StopResonanceTimer()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ResonanceTimerHandle);
	}
}

void URHOnomComponent::HandleResonanceTimerElapsed()
{
	if (ResonanceLayers <= 0)
	{
		StopResonanceTimer();
		return;
	}

	// 衰减按倍率扣（攻击攒的共鸣按当前武器 DA 配置倍率衰减）。
	ResonanceTimeRemaining = FMath::Max(0.f, ResonanceTimeRemaining - 0.1f * ResonanceDecayRate);
	if (ResonanceTimeRemaining <= 0.f)
	{
		ClearResonance(true);
		return;
	}

	FRHOnomResonanceEventData Event;
	Event.OldLayers = ResonanceLayers;
	Event.NewLayers = ResonanceLayers;
	Event.Type = ResonanceType;
	Event.SignedSum = ResonanceSignedSum;
	Event.TimeRemaining = ResonanceTimeRemaining;
	OnResonanceTimeChanged.Broadcast(Event);
	BroadcastState();
}

void URHOnomComponent::BroadcastState() const
{
	OnOnomStateChanged.Broadcast(GetOnomState());
}
