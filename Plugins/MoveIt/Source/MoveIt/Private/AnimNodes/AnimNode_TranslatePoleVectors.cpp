// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.

#include "AnimNodes/AnimNode_TranslatePoleVectors.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/BlendProfile.h"
#include "MIAnimInstanceProxy.h"
#include "MIAnimInstance.h"
#include "Kismet/KismetMathLibrary.h"


void FAnimNode_TranslatePoleVectors::CopyPoleVector(const FMITranslatePoleVectors_PoleVector& Bone, FPoseContext& Output, const FBoneContainer& RequiredBones, float ActualBlendWeight)
{
	ConvertLocalToComponent(Output, Bone.IKIndex);
	FTransform SourceBoneTM = ConvertLocalToComponent(Output, Bone.BoneIndex);

	FTransform ParentTM = ConvertLocalToComponent(Output, Bone.ParentIndex);
	FTransform EffectorTM = ConvertLocalToComponent(Output, Bone.EffectorIndex);

	FTransform JointTargetTM = ConvertLocalToComponent(Output, Bone.IKIndex);
	FVector JointTarget = JointTargetTM.GetTranslation();

	// Get limb lengths
	const float UpperLimbLength = (SourceBoneTM.GetTranslation() - ParentTM.GetTranslation()).Size();
	const float LowerLimbLength = (EffectorTM.GetTranslation() - SourceBoneTM.GetTranslation()).Size();

	// From SolveTwoBoneIK
	FVector EffectorPos = EffectorTM.GetTranslation();
	FVector RootPos = ParentTM.GetTranslation();
	FVector DesiredPos = EffectorPos;
	FVector DesiredDelta = DesiredPos - RootPos;
	float DesiredLength = DesiredDelta.Size();

	float MaxLimbLength = LowerLimbLength + UpperLimbLength;

	// Check to handle case where DesiredPos is the same as RootPos.
	FVector	DesiredDir;
	if (DesiredLength < (float)KINDA_SMALL_NUMBER)
	{
		DesiredLength = (float)KINDA_SMALL_NUMBER;
		DesiredDir = FVector(1, 0, 0);
	}
	else
	{
		DesiredDir = DesiredDelta.GetSafeNormal();
	}

	// Get joint target (used for defining plane that joint should be in).
	FVector JointTargetDelta = JointTarget - RootPos;
	const float JointTargetLengthSqr = JointTargetDelta.SizeSquared();

	// Same check as above, to cover case when JointTarget position is the same as RootPos.
	FVector JointPlaneNormal, JointBendDir;
	if (JointTargetLengthSqr < FMath::Square((float)KINDA_SMALL_NUMBER))
	{
		JointBendDir = FVector(0, 1, 0);
		JointPlaneNormal = FVector(0, 0, 1);
	}
	else
	{
		JointPlaneNormal = DesiredDir ^ JointTargetDelta;

		// If we are trying to point the limb in the same direction that we are supposed to displace the joint in, 
		// we have to just pick 2 random vector perp to DesiredDir and each other.
		if (JointPlaneNormal.SizeSquared() < FMath::Square((float)KINDA_SMALL_NUMBER))
		{
			DesiredDir.FindBestAxisVectors(JointPlaneNormal, JointBendDir);
		}
		else
		{
			JointPlaneNormal.Normalize();

			// Find the final member of the reference frame by removing any component of JointTargetDelta along DesiredDir.
			// This should never leave a zero vector, because we've checked DesiredDir and JointTargetDelta are not parallel.
			JointBendDir = JointTargetDelta - ((JointTargetDelta | DesiredDir) * DesiredDir);
			JointBendDir.Normalize();
		}
	}

	SourceBoneTM.SetTranslation(SourceBoneTM.GetTranslation() + JointBendDir * Bone.PoleVectorOffset);

	ApplyComponentToLocal(Output, SourceBoneTM, Bone.IKIndex, ActualBlendWeight);
}

void FAnimNode_TranslatePoleVectors::EvaluateLocalSkeletalControl_AnyThread(FPoseContext& Output, float ActualBlendWeight)
{
	const FBoneContainer& BoneContainer = Output.AnimInstanceProxy->GetRequiredBones();

	for (const FMITranslatePoleVectors_PoleVector& Bone : PoleVectors)
	{
		CopyPoleVector(Bone, Output, BoneContainer, ActualBlendWeight);
	}
}

bool FAnimNode_TranslatePoleVectors::IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones)
{
	for (FMITranslatePoleVectors_PoleVector& Bone : PoleVectors)
	{
		if (!Bone.Bone.IsValidToEvaluate(RequiredBones)) { return false; }
		if (!Bone.IK.IsValidToEvaluate(RequiredBones)) { return false; }
		if (!Bone.EffectorBone.IsValidToEvaluate(RequiredBones)) { return false; }

		if (Bone.ParentIndex == INDEX_NONE || !RequiredBones.Contains(Bone.ParentIndex.GetInt())) { return false; }
	}

	return true;
}

void FAnimNode_TranslatePoleVectors::InitializeBoneParentCache(const FBoneContainer& BoneContainer)
{
	for (FMITranslatePoleVectors_PoleVector& Bone : PoleVectors)
	{
		InitializeBoneParents(Bone.BoneIndex, BoneContainer);
		InitializeBoneParents(Bone.IKIndex, BoneContainer);
		if (Bone.ParentIndex.GetInt() != INDEX_NONE)
		{
			InitializeBoneParents(Bone.ParentIndex, BoneContainer);
		}
		if (Bone.EffectorIndex.GetInt() != INDEX_NONE)
		{
			InitializeBoneParents(Bone.EffectorIndex, BoneContainer);
		}
	}
}

void FAnimNode_TranslatePoleVectors::InitializeBoneReferences(const FBoneContainer& RequiredBones)
{
	for (FMITranslatePoleVectors_PoleVector& Bone : PoleVectors)
	{
		Bone.Bone.Initialize(RequiredBones);
		Bone.IK.Initialize(RequiredBones);
		Bone.EffectorBone.Initialize(RequiredBones);

		Bone.BoneIndex = Bone.Bone.GetCompactPoseIndex(RequiredBones);
		Bone.IKIndex = Bone.IK.GetCompactPoseIndex(RequiredBones);
		Bone.EffectorIndex = Bone.EffectorBone.GetCompactPoseIndex(RequiredBones);

		if (Bone.BoneIndex.GetInt() != INDEX_NONE)
		{
			Bone.ParentIndex = RequiredBones.GetParentBoneIndex(Bone.BoneIndex);
			if (Bone.ParentIndex.GetInt() != INDEX_NONE)
			{
				RequiredBones.MakeCompactPoseIndex(FMeshPoseBoneIndex(Bone.ParentIndex.GetInt()));
			}
		}
	}
}
