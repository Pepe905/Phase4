// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.

#include "AnimNodes/AnimNode_AdditiveBlendSpace.h"
#include "Animation/AnimInstanceProxy.h"
#include "AnimationRuntime.h"
#include "Animation/BlendSpace.h"

void FAnimNode_AdditiveBlendSpace::Initialize_AnyThread(const FAnimationInitializeContext& Context)
{
	FAnimNode_Base::Initialize_AnyThread(Context);

	SourcePose.Initialize(Context);

	AlphaBoolBlend.Reinitialize();
	AlphaScaleBiasClamp.Reinitialize();
}

void FAnimNode_AdditiveBlendSpace::CacheBones_AnyThread(const FAnimationCacheBonesContext& Context)
{
	SourcePose.CacheBones(Context);
}

void FAnimNode_AdditiveBlendSpace::UpdateAssetPlayer(const FAnimationUpdateContext& Context)
{
	SourcePose.Update(Context);

	//ActualAlpha = 1.f;
	if (IsLODEnabled(Context.AnimInstanceProxy))
	{
		// @note: If you derive from this class, and if you have input that you rely on for base
		// this is not going to work	
		GetEvaluateGraphExposedInputs().Execute(Context);

		switch (AlphaInputType)
		{
		case EAnimAlphaInputType::Float:
			ActualAlpha = AlphaScaleBias.ApplyTo(AlphaScaleBiasClamp.ApplyTo(Alpha, Context.GetDeltaTime()));
			break;
		case EAnimAlphaInputType::Bool:
			ActualAlpha = AlphaBoolBlend.ApplyTo(bAlphaBoolEnabled, Context.GetDeltaTime());
			break;
		case EAnimAlphaInputType::Curve:
			if (UAnimInstance * AnimInstance = Cast<UAnimInstance>(Context.AnimInstanceProxy->GetAnimInstanceObject()))
			{
				ActualAlpha = AlphaScaleBiasClamp.ApplyTo(AnimInstance->GetCurveValue(AlphaCurveName), Context.GetDeltaTime());
			}
			break;
		};

		if (FAnimWeight::IsRelevant(ActualAlpha))
		{
			UpdateInternal(Context);
		}
	}
}

void FAnimNode_AdditiveBlendSpace::Evaluate_AnyThread(FPoseContext& Output)
{
	//@TODO: Could evaluate Base into Output and save a copy
	if (FAnimWeight::IsRelevant(ActualAlpha))
	{
		const bool bExpectsAdditivePose = true;
		FPoseContext AdditiveEvalContext(Output, bExpectsAdditivePose);

		SourcePose.Evaluate(Output);
		FAnimNode_MIBlendSpacePlayer::Evaluate_AnyThread(AdditiveEvalContext);

		FAnimationPoseData OutAnimationPoseData(Output);
		const FAnimationPoseData AdditiveAnimationPoseData(AdditiveEvalContext);

		if (AdditiveType == EMIAdditiveType::MIAS_Additive)
		{
			FAnimationRuntime::AccumulateAdditivePose(OutAnimationPoseData, AdditiveAnimationPoseData, ActualAlpha, AAT_LocalSpaceBase);
		}
		else
		{
			FAnimationRuntime::AccumulateAdditivePose(OutAnimationPoseData, AdditiveAnimationPoseData, ActualAlpha, AAT_RotationOffsetMeshSpace);
		}

		Output.Pose.NormalizeRotations();
	}
	else
	{
		SourcePose.Evaluate(Output);
	}
}

void FAnimNode_AdditiveBlendSpace::GatherDebugData(FNodeDebugData& DebugData)
{
	FString DebugLine = DebugData.GetNodeName(this);
	DebugLine += FString::Printf(TEXT("(Alpha: %.1f%%)"), ActualAlpha * 100.f);

	DebugData.AddDebugItem(DebugLine);
	SourcePose.GatherDebugData(DebugData.BranchFlow(1.f));
	FAnimNode_MIBlendSpacePlayer::GatherDebugData(DebugData);
}

void FAnimNode_AdditiveBlendSpace::UpdateInternal(const FAnimationUpdateContext& Context)
{
	if ((GetBlendSpace() != nullptr) && (Context.AnimInstanceProxy->IsSkeletonCompatible(GetBlendSpace()->GetSkeleton())))
	{
		UpdateBlendSpace(Context);

		X *= ActualAlpha;
		Y *= ActualAlpha;

		FAnimNode_MIBlendSpacePlayer::UpdateInternal(Context);
	}
}
