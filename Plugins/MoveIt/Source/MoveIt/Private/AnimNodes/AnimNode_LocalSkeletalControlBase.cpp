// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.

#include "AnimNodes/AnimNode_LocalSkeletalControlBase.h"
#include "Animation/AnimInstanceProxy.h"
#include "AnimationRuntime.h"

void FAnimNode_LocalSkeletalControlBase::Initialize_AnyThread(const FAnimationInitializeContext& Context)
{
	FAnimNode_Base::Initialize_AnyThread(Context);

	SourcePose.Initialize(Context);

	AlphaBoolBlend.Reinitialize();
	AlphaScaleBiasClamp.Reinitialize();
}

void FAnimNode_LocalSkeletalControlBase::CacheBones_AnyThread(const FAnimationCacheBonesContext& Context)
{
	InitializeBoneReferences(Context.AnimInstanceProxy->GetRequiredBones());
	InitializeBoneParentLUT(Context.AnimInstanceProxy->GetRequiredBones());

	SourcePose.CacheBones(Context);
}

void FAnimNode_LocalSkeletalControlBase::Update_AnyThread(const FAnimationUpdateContext& Context)
{
	GetEvaluateGraphExposedInputs().Execute(Context);
	SourcePose.Update(Context);

	BlendWeight = 0.f;
	if (IsLODEnabled(Context.AnimInstanceProxy))
	{
		GetEvaluateGraphExposedInputs().Execute(Context);

		// Apply the skeletal control if it's valid
		switch (AlphaInputType)
		{
		case EAnimAlphaInputType::Float:
			BlendWeight = AlphaScaleBias.ApplyTo(AlphaScaleBiasClamp.ApplyTo(Alpha, Context.GetDeltaTime()));
			break;
		case EAnimAlphaInputType::Bool:
			BlendWeight = AlphaBoolBlend.ApplyTo(bAlphaBoolEnabled, Context.GetDeltaTime());
			break;
		case EAnimAlphaInputType::Curve:
			if (UAnimInstance * AnimInstance = Cast<UAnimInstance>(Context.AnimInstanceProxy->GetAnimInstanceObject()))
			{
				BlendWeight = AlphaScaleBiasClamp.ApplyTo(AnimInstance->GetCurveValue(AlphaCurveName), Context.GetDeltaTime());
			}
			break;
		};

		// Make sure Alpha is clamped between 0 and 1.
		BlendWeight = FMath::Clamp<float>(BlendWeight, 0.f, 1.f);

		if (FAnimWeight::IsRelevant(BlendWeight) && IsValidToEvaluate(Context.AnimInstanceProxy->GetSkeleton(), Context.AnimInstanceProxy->GetRequiredBones()))
		{
			UpdateInternal(Context);
		}
	}
}

void FAnimNode_LocalSkeletalControlBase::Evaluate_AnyThread(FPoseContext& Output)
{
	EvaluatePose_AnyThread(Output);

#if DO_CHECK
	// this is to ensure Source data does not contain NaN
	ensure(Output.ContainsNaN() == false);
#endif

	// Apply the skeletal control if it's valid
	if (FAnimWeight::IsRelevant(BlendWeight) && IsValidToEvaluate(Output.AnimInstanceProxy->GetSkeleton(), Output.AnimInstanceProxy->GetRequiredBones()))
	{
		const float ActualBlendWeight = FMath::Clamp<float>(BlendWeight, 0.f, 1.f);
		EvaluateLocalSkeletalControl_AnyThread(Output, ActualBlendWeight);
	}
}

void FAnimNode_LocalSkeletalControlBase::EvaluatePose_AnyThread(FPoseContext& Output)
{
	SourcePose.Evaluate(Output);
}

FTransform FAnimNode_LocalSkeletalControlBase::ConvertLocalToComponent(FPoseContext& Output, const FCompactPoseBoneIndex BoneIndex)
{
	FBoneLookupTable* LUT = BoneLUT.FindByKey(BoneIndex);
	if (!LUT)
	{
		// Edge case from compiling anim instance (or you didn't init parent bone references!)
		return FTransform::Identity;
	}
	return LUT->ConvertLocalToComponent(Output);
}

FTransform FAnimNode_LocalSkeletalControlBase::ConvertComponentToLocal(FTransform& TargetComponentSpace, const FCompactPoseBoneIndex BoneIndex)
{
	FBoneLookupTable* LUT = BoneLUT.FindByKey(BoneIndex);
	return LUT->ConvertComponentToLocal(TargetComponentSpace);
}

void FAnimNode_LocalSkeletalControlBase::ApplyComponentToLocal(FPoseContext& Output, FTransform& TargetComponentSpace, const FCompactPoseBoneIndex BoneIndex, float InAlpha)
{
	const FTransform InitialTransform = Output.Pose[BoneIndex];
	ConvertComponentToLocal(TargetComponentSpace, BoneIndex);
	
	if (!FAnimationRuntime::IsFullWeight(InAlpha))
	{
		// Blend it back by the alpha if needed
		Output.Pose[BoneIndex].BlendWith(InitialTransform, (1.f - InAlpha));
	}
}

void FAnimNode_LocalSkeletalControlBase::InitializeBoneParents(FBoneReference& BoneRef, const FBoneContainer& BoneContainer)
{
	if (!BoneRef.IsValidToEvaluate(BoneContainer))
	{
		return;
	}
	InitializeBoneParents(BoneRef.GetCompactPoseIndex(BoneContainer), BoneContainer);
}

void FAnimNode_LocalSkeletalControlBase::InitializeBoneParents(const FCompactPoseBoneIndex& BoneIndex, const FBoneContainer& BoneContainer)
{
	if (BoneIndex == INDEX_NONE)
	{
		return;
	}

	FBoneLookupTable& ParentCache = BoneLUT.Add_GetRef(FBoneLookupTable());
	ParentCache.BoneIndex = BoneIndex;

	FCompactPoseBoneIndex ParentBoneIndex = BoneContainer.GetParentBoneIndex(BoneIndex);
	// Cache parents
	while (ParentBoneIndex >= 0)
	{
		ParentCache.Cache.Add(ParentBoneIndex);
		ParentBoneIndex = BoneContainer.GetParentBoneIndex(ParentBoneIndex);
	};

	if (ParentCache.Cache.Num() > 0)
	{
		ParentCache.Cache.Sort();
	}
}

void FAnimNode_LocalSkeletalControlBase::AddDebugNodeData(FString& OutDebugData)
{
	OutDebugData += FString::Printf(TEXT("Alpha: %.1f%%"), BlendWeight * 100.f);
}

void FAnimNode_LocalSkeletalControlBase::InitializeBoneParentLUT(const FBoneContainer& BoneContainer)
{
	for (FBoneLookupTable& ParentCache : BoneLUT)
	{
		ParentCache.Cache.Reset();
	}

	BoneLUT.Reset();

	InitializeBoneParentCache(BoneContainer);

	if (BoneLUT.Num() > 0)
	{
		BoneLUT.Sort();
	}
}
