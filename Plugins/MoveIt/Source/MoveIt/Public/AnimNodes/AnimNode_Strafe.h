// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.

#pragma once

#include "AnimNodes/AnimNode_LocalSkeletalControlBase.h"
#include "MITypes.h"
#include "AnimNode_Strafe.generated.h"


/**
 *	Procedurally generates strafing animations by reorienting the feet and body
 *	Applies intelligent logic from animation instance to change orientations
 */
USTRUCT(BlueprintInternalUseOnly)
struct MOVEIT_API FAnimNode_Strafe : public FAnimNode_LocalSkeletalControlBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = Strafe)
	FBoneReference FootIKRootBone;

	UPROPERTY(EditAnywhere, Category = Strafe)
	FBoneReference PelvisBone;

	/** Optionally add spine bones. Result is averaged between all added bones. Add multiple of the same bone to give it more influence. */
	UPROPERTY(EditAnywhere, Category = Strafe)
	TArray<FBoneReference> SpineBones;

	/** Optionally add head/neck bones. Result is averaged between all added bones. Add multiple of the same bone to give it more influence. */
	UPROPERTY(EditAnywhere, Category = Strafe)
	TArray<FBoneReference> HeadBones;

	/** Strafe direction, computed in MIAnimInstance */
	UPROPERTY(EditAnywhere, Category = Strafe, meta = (PinShownByDefault))
	FVector Direction;

	/** Strafe orientation, computed in MIAnimInstance */
	UPROPERTY(EditAnywhere, Category = Strafe, meta = (PinShownByDefault))
	EMIStrafeOrientation Orientation;

	/** Scaling for foot orientation */
	UPROPERTY(EditAnywhere, Category = Strafe, meta = (PinHiddenByDefault))
	float FootOrientScale;

	/** Scaling for pelvis orientation */
	UPROPERTY(EditAnywhere, Category = Strafe, meta = (PinHiddenByDefault))
	float PelvisOrientScale;

	/** Scaling for body orientation */
	UPROPERTY(EditAnywhere, Category = Strafe, meta = (PinHiddenByDefault))
	float BodyOrientScale;

	/** Scaling for head orientation */
	UPROPERTY(EditAnywhere, Category = Strafe, meta = (PinHiddenByDefault))
	float HeadOrientScale;

protected:
	/** World-space location of the bone. */
	FVector BoneLocation;

	/** World-space velocity of the bone. */
	FVector BoneVelocity;

	/** Velocity of the owning actor */
	FVector OwnerVelocity;

	float DeltaTime;

	FCompactPoseBoneIndex PelvisBoneIndex;
	FCompactPoseBoneIndex FootIKRootBoneIndex;

public:
	FAnimNode_Strafe()
		: FootIKRootBone(FBoneReference("ik_foot_root"))
		, PelvisBone(FBoneReference("pelvis"))
		, SpineBones({ FBoneReference("spine_01"), FBoneReference("spine_02"), FBoneReference("spine_03") })
		, HeadBones({ FBoneReference("neck_01"), FBoneReference("head") })
		, Direction(FVector::ZeroVector)
		, Orientation(EMIStrafeOrientation::SO_Neutral)
		, FootOrientScale(1.f)
		, PelvisOrientScale(0.5f)
		, BodyOrientScale(0.5f)
		, HeadOrientScale(0.5f)
		, BoneLocation(FVector::ZeroVector)
		, BoneVelocity(FVector::ZeroVector)
		, OwnerVelocity(FVector::ZeroVector)
		, DeltaTime(0.f)
		, PelvisBoneIndex(-1)
		, FootIKRootBoneIndex(-1)
	{
		AlphaInputType = EAnimAlphaInputType::Bool;
		AlphaBoolBlend.BlendInTime = 0.2f;
		AlphaBoolBlend.BlendOutTime = 0.2f;
		AlphaBoolBlend.BlendOption = EAlphaBlendOption::HermiteCubic;
	}

protected:
	// FAnimNode_LocalSkeletalControlBase interface
	virtual void UpdateInternal(const FAnimationUpdateContext& Context) final;
	virtual void EvaluateLocalSkeletalControl_AnyThread(FPoseContext& Output, float BlendWeight) final;
	virtual bool IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) final;
	// End of FAnimNode_LocalSkeletalControlBase interface

	virtual void InitializeBoneParentCache(const FBoneContainer& BoneContainer) final;
	virtual void InitializeBoneReferences(const FBoneContainer& RequiredBones) final;
};