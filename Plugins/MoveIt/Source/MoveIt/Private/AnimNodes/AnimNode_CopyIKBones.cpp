// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.

#include "AnimNodes/AnimNode_CopyIKBones.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/BlendProfile.h"
#include "MIAnimInstanceProxy.h"
#include "MIAnimInstance.h"
#include "Kismet/KismetMathLibrary.h"

DEFINE_LOG_CATEGORY_STATIC(LogCopyBones, Log, All);

void FAnimNode_CopyIKBones::CopyBones(const FMICopyIKBones_Bone& Bone, FPoseContext& Output, const FBoneContainer& RequiredBones, float ActualBlendWeight)
{
	ConvertLocalToComponent(Output, Bone.IKIndex);
	FTransform SourceBoneTM = ConvertLocalToComponent(Output, Bone.BoneIndex);

	ApplyComponentToLocal(Output, SourceBoneTM, Bone.IKIndex, ActualBlendWeight);
}

void FAnimNode_CopyIKBones::EvaluateLocalSkeletalControl_AnyThread(FPoseContext& Output, float ActualBlendWeight)
{
	const FBoneContainer& BoneContainer = Output.AnimInstanceProxy->GetRequiredBones();

	for (const FMICopyIKBones_Bone& Bone : Bones)
	{
		CopyBones(Bone, Output, BoneContainer, ActualBlendWeight);
	}
}

bool FAnimNode_CopyIKBones::IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones)
{
	for (FMICopyIKBones_Bone& Bone : Bones)
	{
		if (!Bone.Bone.IsValidToEvaluate(RequiredBones)) { return false; }
		if (!Bone.IK.IsValidToEvaluate(RequiredBones)) { return false; }
	}

	return true;
}

void FAnimNode_CopyIKBones::InitializeBoneParentCache(const FBoneContainer& BoneContainer)
{
	for (FMICopyIKBones_Bone& Bone : Bones)
	{
		InitializeBoneParents(Bone.BoneIndex, BoneContainer);
		InitializeBoneParents(Bone.IKIndex, BoneContainer);
	}
}

void FAnimNode_CopyIKBones::InitializeBoneReferences(const FBoneContainer& RequiredBones)
{
	for (FMICopyIKBones_Bone& Bone : Bones)
	{
		Bone.Bone.Initialize(RequiredBones);
		Bone.IK.Initialize(RequiredBones);

		Bone.BoneIndex = Bone.Bone.GetCompactPoseIndex(RequiredBones);
		Bone.IKIndex = Bone.IK.GetCompactPoseIndex(RequiredBones);
	}
}
