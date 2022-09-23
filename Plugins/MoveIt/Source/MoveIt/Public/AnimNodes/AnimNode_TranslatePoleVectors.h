// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.

#pragma once

#include "AnimNodes/AnimNode_LocalSkeletalControlBase.h"
#include "AnimNodes/AnimNodes_Shared.h"
#include "AnimNode_TranslatePoleVectors.generated.h"


USTRUCT(BlueprintType)
struct FMITranslatePoleVectors_PoleVector : public FMICopyIKBones_Bone
{
	GENERATED_BODY()

	FMITranslatePoleVectors_PoleVector(const FBoneReference& InBone, const FBoneReference& InIK, const FBoneReference& InEffectorBone, float InPoleVectorOffset = 100.f)
		: FMICopyIKBones_Bone(InBone, InIK)
		, EffectorBone(InEffectorBone)
		, PoleVectorOffset(InPoleVectorOffset)
		, ParentIndex(-1)
		, EffectorIndex(-1)
	{}

	FMITranslatePoleVectors_PoleVector()
		: FMICopyIKBones_Bone()
		, PoleVectorOffset(100.f)
		, ParentIndex(-1)
		, EffectorIndex(-1)
	{}

	UPROPERTY(EditAnywhere, Category = "Copy IK Bones")
	FBoneReference EffectorBone;

	UPROPERTY(EditAnywhere, Category = "Copy IK Bones")
	float PoleVectorOffset;

	FCompactPoseBoneIndex ParentIndex;
	FCompactPoseBoneIndex EffectorIndex;
};

/**
 *	Compute and apply ideal pole vector location for arms
 */
USTRUCT(BlueprintInternalUseOnly)
struct MOVEIT_API FAnimNode_TranslatePoleVectors : public FAnimNode_LocalSkeletalControlBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Copy IK Bones")
	TArray<FMITranslatePoleVectors_PoleVector> PoleVectors;

protected:
	bool bValid;

public:
	FAnimNode_TranslatePoleVectors()
		: PoleVectors({
			FMITranslatePoleVectors_PoleVector(FBoneReference("lowerarm_l"), FBoneReference("VB upperarm_l_lowerarm_l"), FBoneReference("hand_l")),
			FMITranslatePoleVectors_PoleVector(FBoneReference("lowerarm_r"), FBoneReference("VB upperarm_r_lowerarm_r"), FBoneReference("hand_r"))
		})
		, bValid(true)
	{}

protected:
	void CopyPoleVector(const FMITranslatePoleVectors_PoleVector& Bone, FPoseContext& Output, const FBoneContainer& RequiredBones, float BlendWeight);

	// FAnimNode_LocalSkeletalControlBase interface
	virtual void EvaluateLocalSkeletalControl_AnyThread(FPoseContext& Output, float BlendWeight) final;
	virtual bool IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) final;
	// End of FAnimNode_LocalSkeletalControlBase interface

	virtual void InitializeBoneParentCache(const FBoneContainer& BoneContainer) final;
	virtual void InitializeBoneReferences(const FBoneContainer& RequiredBones) final;
};