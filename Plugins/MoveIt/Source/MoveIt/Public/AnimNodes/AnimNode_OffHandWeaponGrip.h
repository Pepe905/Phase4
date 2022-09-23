// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.

#pragma once

#include "BoneControllers/AnimNode_SkeletalControlBase.h"
#include "MITypes.h"
#include "AnimNode_OffHandWeaponGrip.generated.h"


/**
 *	Procedurally places the off-hand on a weapon
 */
USTRUCT(BlueprintInternalUseOnly)
struct MOVEIT_API FAnimNode_OffHandWeaponGrip : public FAnimNode_SkeletalControlBase
{
	GENERATED_BODY()

public:
	/** Bone holding weapon (not IK) */
	UPROPERTY(EditAnywhere, Category = IK)
	FBoneReference WeaponHandBone;

	/** Bone used for OffHand IK */
	UPROPERTY(EditAnywhere, Category = IK)
	FBoneReference OffHandIKBone;

	/** Weapon character is currently holding */
	UPROPERTY(EditAnywhere, Category = OffHand, meta = (PinShownByDefault))
	FMIWeapon Weapon;

protected:
	FCompactPoseBoneIndex OffHandPoseIndex;

	bool bValidProxy;

public:
	FAnimNode_OffHandWeaponGrip()
		: WeaponHandBone(FBoneReference("hand_r"))
		, OffHandIKBone(FBoneReference("ik_hand_l"))
		, OffHandPoseIndex(-1)
		, bValidProxy(false)
	{
		AlphaInputType = EAnimAlphaInputType::Bool;
		AlphaBoolBlend.BlendInTime = 0.2f;
		AlphaBoolBlend.BlendOutTime = 0.2f;
		AlphaBoolBlend.BlendOption = EAlphaBlendOption::HermiteCubic;
	}

	// FAnimNode_Base interface
	virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) final;
	// End of FAnimNode_Base interface

	// FAnimNode_SkeletalControlBase interface
	virtual void EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms) final;
	virtual bool IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) final;
	// End of FAnimNode_SkeletalControlBase interface

	void OrientMeshToGround_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms);

private:
	// FAnimNode_SkeletalControlBase interface
	virtual void InitializeBoneReferences(const FBoneContainer& RequiredBones) final;
	// End of FAnimNode_SkeletalControlBase interface
};