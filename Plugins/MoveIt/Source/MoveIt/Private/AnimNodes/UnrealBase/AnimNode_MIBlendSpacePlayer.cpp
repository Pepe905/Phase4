// Copyright Epic Games, Inc. All Rights Reserved.

#include "AnimNodes/UnrealBase/AnimNode_MIBlendSpacePlayer.h"
#include "Animation/BlendSpace.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimInstanceProxy.h"
#include "AnimGraphRuntimeTrace.h"
#include "Animation/AnimSync.h"
#include "Animation/AnimSyncScope.h"

/////////////////////////////////////////////////////
// FAnimNode_MIBlendSpacePlayer

float FAnimNode_MIBlendSpacePlayer::GetCurrentAssetTime() const
{
	if(const FBlendSampleData* HighestWeightedSample = GetHighestWeightedSample())
	{
		return HighestWeightedSample->Time;
	}

	// No sample
	return 0.0f;
}

float FAnimNode_MIBlendSpacePlayer::GetCurrentAssetTimePlayRateAdjusted() const
{
	const float Length = GetCurrentAssetLength();
	return GetPlayRate() < 0.0f ? Length - InternalTimeAccumulator * Length : Length * InternalTimeAccumulator;
}

float FAnimNode_MIBlendSpacePlayer::GetCurrentAssetLength() const
{
	if(const FBlendSampleData* HighestWeightedSample = GetHighestWeightedSample())
	{
		const UBlendSpace* CurrentBlendSpace = GetBlendSpace();
		if (CurrentBlendSpace != nullptr)
		{
			const FBlendSample& Sample = CurrentBlendSpace->GetBlendSample(HighestWeightedSample->SampleDataIndex);
			return Sample.Animation->GetPlayLength();
		}
	}

	// No sample
	return 0.0f;
}

void FAnimNode_MIBlendSpacePlayer::Initialize_AnyThread(const FAnimationInitializeContext& Context)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(Initialize_AnyThread)
	FAnimNode_AssetPlayerBase::Initialize_AnyThread(Context);

	GetEvaluateGraphExposedInputs().Execute(Context);

	Reinitialize();

	PreviousBlendSpace = GetBlendSpace();
}

void FAnimNode_MIBlendSpacePlayer::CacheBones_AnyThread(const FAnimationCacheBonesContext& Context)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(CacheBones_AnyThread)
}

void FAnimNode_MIBlendSpacePlayer::UpdateAssetPlayer(const FAnimationUpdateContext& Context)
{
	GetEvaluateGraphExposedInputs().Execute(Context);

	UpdateInternal(Context);
}

void FAnimNode_MIBlendSpacePlayer::UpdateInternal(const FAnimationUpdateContext& Context)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(UpdateInternal)

	UBlendSpace* CurrentBlendSpace = GetBlendSpace();
	if ((CurrentBlendSpace != nullptr) && (Context.AnimInstanceProxy->IsSkeletonCompatible(CurrentBlendSpace->GetSkeleton())))
	{	
		if (PreviousBlendSpace != CurrentBlendSpace)
		{
			Reinitialize(ShouldResetPlayTimeWhenBlendSpaceChanges());
		}

		const FVector Position = GetPosition();

		// Create a tick record and push into the closest scope
		UE::Anim::FAnimSyncGroupScope& SyncScope = Context.GetMessageChecked<UE::Anim::FAnimSyncGroupScope>();

		FAnimTickRecord TickRecord(
			CurrentBlendSpace, Position, BlendSampleDataCache, BlendFilter, GetLoop(), GetPlayRate(), ShouldTeleportToTime(), 
			IsEvaluator(), Context.GetFinalBlendWeight(), /*inout*/ InternalTimeAccumulator, MarkerTickRecord);
		TickRecord.RootMotionWeightModifier = Context.GetRootMotionWeightModifier();
		TickRecord.DeltaTimeRecord = &DeltaTimeRecord;

		const UE::Anim::FAnimSyncParams SyncParams(GetGroupName(), GetGroupRole(), GetGroupMethod());
		TickRecord.GatherContextData(Context);

		SyncScope.AddTickRecord(TickRecord, SyncParams, UE::Anim::FAnimSyncDebugInfo(Context));

		TRACE_ANIM_TICK_RECORD(Context, TickRecord);

#if WITH_EDITORONLY_DATA
		if (FAnimBlueprintDebugData* DebugData = Context.AnimInstanceProxy->GetAnimBlueprintDebugData())
		{
			DebugData->RecordBlendSpacePlayer(Context.GetCurrentNodeId(), CurrentBlendSpace, Position, BlendFilter.GetFilterLastOutput());
		}
#endif

		PreviousBlendSpace = CurrentBlendSpace;
	}

	TRACE_ANIM_NODE_VALUE(Context, TEXT("Name"), CurrentBlendSpace ? *CurrentBlendSpace->GetName() : TEXT("None"));
	TRACE_ANIM_NODE_VALUE(Context, TEXT("Blend Space"), CurrentBlendSpace);
	TRACE_ANIM_NODE_VALUE(Context, TEXT("Playback Time"), InternalTimeAccumulator);
}

void FAnimNode_MIBlendSpacePlayer::Evaluate_AnyThread(FPoseContext& Output)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(Evaluate_AnyThread)

	const UBlendSpace* CurrentBlendSpace = GetBlendSpace();
	if ((CurrentBlendSpace != nullptr) && (Output.AnimInstanceProxy->IsSkeletonCompatible(CurrentBlendSpace->GetSkeleton())))
	{
		FAnimationPoseData AnimationPoseData(Output);
		CurrentBlendSpace->GetAnimationPose(BlendSampleDataCache, FAnimExtractContext(InternalTimeAccumulator, Output.AnimInstanceProxy->ShouldExtractRootMotion(), DeltaTimeRecord, GetLoop()), AnimationPoseData);
	}
	else
	{
		Output.ResetToRefPose();
	}
}

void FAnimNode_MIBlendSpacePlayer::GatherDebugData(FNodeDebugData& DebugData)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(GatherDebugData)
	FString DebugLine = DebugData.GetNodeName(this);

	if (const UBlendSpace* CurrentBlendSpace = GetBlendSpace())
	{
		DebugLine += FString::Printf(TEXT("('%s' Play Time: %.3f)"), *CurrentBlendSpace->GetName(), InternalTimeAccumulator);

		DebugData.AddDebugItem(DebugLine, true);
	}
}

float FAnimNode_MIBlendSpacePlayer::GetTimeFromEnd(float CurrentTime) const
{
	// Blend-spaces use normalized time value
	constexpr float PlayLength = 1.0f;
	return GetBlendSpace() != nullptr ? PlayLength - CurrentTime : 0.0f;
}

UAnimationAsset* FAnimNode_MIBlendSpacePlayer::GetAnimAsset() const
{
	return GetBlendSpace();
}

const FBlendSampleData* FAnimNode_MIBlendSpacePlayer::GetHighestWeightedSample() const
{
	if(BlendSampleDataCache.Num() == 0)
	{
		return nullptr;
	}

	const FBlendSampleData* HighestSample = &BlendSampleDataCache[0];

	for(int32 Idx = 1; Idx < BlendSampleDataCache.Num(); ++Idx)
	{
		if(BlendSampleDataCache[Idx].TotalWeight > HighestSample->TotalWeight)
		{
			HighestSample = &BlendSampleDataCache[Idx];
		}
	}

	return HighestSample;
}

void FAnimNode_MIBlendSpacePlayer::Reinitialize(bool bResetTime)
{
	BlendSampleDataCache.Empty();
	if(bResetTime)
	{
		const float CurrentStartPosition = GetStartPosition();

		InternalTimeAccumulator = FMath::Clamp(CurrentStartPosition, 0.f, 1.0f);
		if (CurrentStartPosition == 0.f && GetPlayRate() < 0.0f)
		{
			// Blend spaces run between 0 and 1
			InternalTimeAccumulator = 1.0f;
		}
	}

	const UBlendSpace* CurrentBlendSpace = GetBlendSpace();
	if (CurrentBlendSpace != nullptr)
	{
		CurrentBlendSpace->InitializeFilter(&BlendFilter);
	}
}

bool FAnimNode_MIBlendSpacePlayer::SetBlendSpace(UBlendSpace* InBlendSpace)
{
#if WITH_EDITORONLY_DATA
	BlendSpace = InBlendSpace;
	GET_MUTABLE_ANIM_NODE_DATA(TObjectPtr<UBlendSpace>, BlendSpace) = InBlendSpace;
#endif
	
	if(TObjectPtr<UBlendSpace>* BlendSpacePtr = GET_INSTANCE_ANIM_NODE_DATA_PTR(TObjectPtr<UBlendSpace>, BlendSpace))
	{
		*BlendSpacePtr = InBlendSpace;
		return true;
	}

	return false;
}

FVector FAnimNode_MIBlendSpacePlayer::GetPosition() const
{
	return FVector(X, Y, 0.f);
	// return FVector(GET_ANIM_NODE_DATA(float, X), GET_ANIM_NODE_DATA(float, Y), 0.0f);
}

float FAnimNode_MIBlendSpacePlayer::GetPlayRate() const
{
	return GET_ANIM_NODE_DATA(float, PlayRate);
}

bool FAnimNode_MIBlendSpacePlayer::GetLoop() const
{
	return GET_ANIM_NODE_DATA(bool, bLoop);
}

bool FAnimNode_MIBlendSpacePlayer::ShouldResetPlayTimeWhenBlendSpaceChanges() const
{
	return GET_ANIM_NODE_DATA(bool, bResetPlayTimeWhenBlendSpaceChanges);
}

float FAnimNode_MIBlendSpacePlayer::GetStartPosition() const
{
	return GET_ANIM_NODE_DATA(float, StartPosition);
}

UBlendSpace* FAnimNode_MIBlendSpacePlayer::GetBlendSpace() const
{
	return GET_ANIM_NODE_DATA(TObjectPtr<UBlendSpace>, BlendSpace);
}

FName FAnimNode_MIBlendSpacePlayer::GetGroupName() const
{
	return GET_ANIM_NODE_DATA(FName, GroupName);
}

EAnimGroupRole::Type FAnimNode_MIBlendSpacePlayer::GetGroupRole() const
{
	return GET_ANIM_NODE_DATA(TEnumAsByte<EAnimGroupRole::Type>, GroupRole);
}

EAnimSyncMethod FAnimNode_MIBlendSpacePlayer::GetGroupMethod() const
{
	return GET_ANIM_NODE_DATA(EAnimSyncMethod, Method);
}

bool FAnimNode_MIBlendSpacePlayer::GetIgnoreForRelevancyTest() const
{
	return GET_ANIM_NODE_DATA(bool, bIgnoreForRelevancyTest);
}

bool FAnimNode_MIBlendSpacePlayer::SetGroupName(FName InGroupName)
{
#if WITH_EDITORONLY_DATA
	GroupName = InGroupName;
#endif

	if(FName* GroupNamePtr = GET_INSTANCE_ANIM_NODE_DATA_PTR(FName, GroupName))
	{
		*GroupNamePtr = InGroupName;
		return true;
	}

	return false;
}

bool FAnimNode_MIBlendSpacePlayer::SetGroupRole(EAnimGroupRole::Type InRole)
{
#if WITH_EDITORONLY_DATA
	GroupRole = InRole;
#endif
	
	if(TEnumAsByte<EAnimGroupRole::Type>* GroupRolePtr = GET_INSTANCE_ANIM_NODE_DATA_PTR(TEnumAsByte<EAnimGroupRole::Type>, GroupRole))
	{
		*GroupRolePtr = InRole;
		return true;
	}

	return false;
}

bool FAnimNode_MIBlendSpacePlayer::SetGroupMethod(EAnimSyncMethod InMethod)
{
#if WITH_EDITORONLY_DATA
	Method = InMethod;
#endif

	if(EAnimSyncMethod* MethodPtr = GET_INSTANCE_ANIM_NODE_DATA_PTR(EAnimSyncMethod, Method))
	{
		*MethodPtr = InMethod;
		return true;
	}

	return false;
}

bool FAnimNode_MIBlendSpacePlayer::SetIgnoreForRelevancyTest(bool bInIgnoreForRelevancyTest)
{
#if WITH_EDITORONLY_DATA
	bIgnoreForRelevancyTest = bInIgnoreForRelevancyTest;
#endif

	if(bool* bIgnoreForRelevancyTestPtr = GET_INSTANCE_ANIM_NODE_DATA_PTR(bool, bIgnoreForRelevancyTest))
	{
		*bIgnoreForRelevancyTestPtr = bInIgnoreForRelevancyTest;
		return true;
	}

	return false;
}