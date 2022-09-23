// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.

#pragma once

#include "AnimNodes/AnimNode_LocalSkeletalControlBase.h"
#include "AnimNodes/AnimNodes_Shared.h"
#include "AnimNode_AimOffsetCompensation.generated.h"


struct FMIAimOffsetBone
{
	FBoneReference Bone;
	FCompactPoseBoneIndex Index;

	FMIAimOffsetBone()
		: Bone(FBoneReference(NAME_None))
		, Index(INDEX_NONE)
	{}

	FMIAimOffsetBone(const FBoneReference& InBone, const FCompactPoseBoneIndex& InIndex)
		: Bone(InBone)
		, Index(InIndex)
	{}
};

/**
 *	Rotates bones to match the pose negated by Turn In Place so that Aim Offset lines up.
 */
USTRUCT(BlueprintInternalUseOnly)
struct MOVEIT_API FAnimNode_AimOffsetCompensation : public FAnimNode_LocalSkeletalControlBase
{
	GENERATED_BODY()

public:
	/** Which bones will be rotated to compensate for the turn in place */
	UPROPERTY(EditAnywhere, Category = "Aim Offset Compensation")
	TArray<FBoneReference> Bones;

	/** The axis that the twist bones aim down (X by default) - changing this will change which axis compensates and should only be done for non-UE4 characters */
	UPROPERTY(EditAnywhere, Category = "Aim Offset Compensation", meta=(Hidden="None"))
	TEnumAsByte<EAxis::Type> AimAxis;

protected:
	TArray<FMIAimOffsetBone> BoneCache;

	float AimOffsetTurn;

public:
	FAnimNode_AimOffsetCompensation()
		: Bones({
			FBoneReference("spine_01"),
			FBoneReference("spine_02"),
			FBoneReference("spine_03")
		})
		, AimAxis(EAxis::X)
		, AimOffsetTurn(0.f)
	{
		AlphaInputType = EAnimAlphaInputType::Bool;
		AlphaBoolBlend.BlendInTime = 0.2f;
		AlphaBoolBlend.BlendOutTime = 0.2f;
		AlphaBoolBlend.BlendOption = EAlphaBlendOption::HermiteCubic;
	}

protected:
	// FAnimNode_Base interface
	virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) final;
	virtual bool HasPreUpdate() const final { return true; }
	virtual void PreUpdate(const UAnimInstance* InAnimInstance) final;
	// End of FAnimNode_Base interface

	// FAnimNode_LocalSkeletalControlBase interface
	virtual void EvaluateLocalSkeletalControl_AnyThread(FPoseContext& Output, float BlendWeight) final;
	virtual bool IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) final;
	// End of FAnimNode_LocalSkeletalControlBase interface

	virtual void InitializeBoneParentCache(const FBoneContainer& BoneContainer) final;
	virtual void InitializeBoneReferences(const FBoneContainer& RequiredBones) final;
};