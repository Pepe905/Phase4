// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.

#pragma once

#include "AnimNodes/AnimNode_LocalSkeletalControlBase.h"
#include "AnimNodes/AnimNodes_Shared.h"
#include "AnimNode_CopyIKBones.generated.h"


/**
 *	Copies the IK bones to the matching bones. This helps a lot with animations that didn't animate the IK bones to match.
 */
USTRUCT(BlueprintInternalUseOnly)
struct MOVEIT_API FAnimNode_CopyIKBones : public FAnimNode_LocalSkeletalControlBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Copy IK Bones")
	TArray<FMICopyIKBones_Bone> Bones;

public:
	FAnimNode_CopyIKBones()
		: Bones({
			FMICopyIKBones_Bone(FBoneReference("foot_l"), FBoneReference("ik_foot_l")),
			FMICopyIKBones_Bone(FBoneReference("foot_r"), FBoneReference("ik_foot_r")),
			FMICopyIKBones_Bone(FBoneReference("calf_l"), FBoneReference("VB thigh_l_calf_l")), 
			FMICopyIKBones_Bone(FBoneReference("calf_r"), FBoneReference("VB thigh_r_calf_r")),
			FMICopyIKBones_Bone(FBoneReference("hand_l"), FBoneReference("ik_hand_l")),
			FMICopyIKBones_Bone(FBoneReference("hand_r"), FBoneReference("ik_hand_r"))
		})
	{}

protected:
	void CopyBones(const FMICopyIKBones_Bone& Bone, FPoseContext& Output, const FBoneContainer& RequiredBones, float BlendWeight);

	// FAnimNode_LocalSkeletalControlBase interface
	virtual void EvaluateLocalSkeletalControl_AnyThread(FPoseContext& Output, float BlendWeight) final;
	virtual bool IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) final;
	// End of FAnimNode_LocalSkeletalControlBase interface

	virtual void InitializeBoneParentCache(const FBoneContainer& BoneContainer) final;
	virtual void InitializeBoneReferences(const FBoneContainer& RequiredBones) final;
};