// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.

#pragma once

#include "AnimNodes/AnimNode_LocalSkeletalControlBase.h"
#include "AnimNodes/AnimNodes_Shared.h"
#include "AnimNode_TraversalSpring.generated.h"

class AMICharacter;

/**
 *	Applies a spring based on movement while on an incline to push the pelvis up (walking uphill) or down (walking downhill)
 */
USTRUCT(BlueprintInternalUseOnly)
struct MOVEIT_API FAnimNode_TraversalSpring : public FAnimNode_LocalSkeletalControlBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = TraversalSpring)
	FBoneReference Bone;

	/** Spring strength (displacement, mass) when moving up a hill */
	UPROPERTY(EditAnywhere, Category = TraversalSpring)
	float SpringUpwardStrength;

	/** Spring strength (displacement, mass) when moving down a hill */
	UPROPERTY(EditAnywhere, Category = TraversalSpring)
	float SpringDownwardStrength;

	/** Spring stiffness (tightness) */
	UPROPERTY(EditAnywhere, Category = TraversalSpring)
	float SpringStiffness;

	UPROPERTY(EditAnywhere, Category = TraversalSpring)
	float SpringDamping;
	
protected:
	/** Internal use - Amount of time we need to simulate. */
	float RemainingTime;

	/** Internal use - Current timestep */
	float FixedTimeStep;

	/** Internal use - Current time dilation */
	float TimeDilation;

	/** Velocity of the owning actor */
	FVector OwnerVelocity;

	FVector BoneVelocity;

	FVector BoneLocation;

	FCompactPoseBoneIndex BoneIndex;

	AMICharacter* Character;

public:
	FAnimNode_TraversalSpring()
		: Bone(FBoneReference("pelvis"))
		, SpringUpwardStrength(1.2f)
		, SpringDownwardStrength(1.f)
		, SpringStiffness(2.f)
		, SpringDamping(200.f)
		, RemainingTime(0.f)
		, FixedTimeStep(0.f)
		, TimeDilation(1.f)
		, OwnerVelocity(FVector::ZeroVector)
		, BoneVelocity(FVector::ZeroVector)
		, BoneLocation(FVector::ZeroVector)
		, BoneIndex(-1)
		, Character(nullptr)
	{
		AlphaInputType = EAnimAlphaInputType::Bool;
		AlphaBoolBlend.BlendInTime = 0.2f;
		AlphaBoolBlend.BlendOutTime = 0.f;
		AlphaBoolBlend.BlendOption = EAlphaBlendOption::HermiteCubic;
	}

public:
	// FAnimNode_Base interface
	virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) final;
	virtual void PreUpdate(const UAnimInstance* InAnimInstance) final;
	virtual bool HasPreUpdate() const final { return true; }
	// End of FAnimNode_Base interface

protected:
	// FAnimNode_LocalSkeletalControlBase interface
	virtual void UpdateInternal(const FAnimationUpdateContext& Context) final;
	virtual void EvaluateLocalSkeletalControl_AnyThread(FPoseContext& Output, float BlendWeight) final;
	virtual bool IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) final;
	// End of FAnimNode_LocalSkeletalControlBase interface

	virtual void InitializeBoneParentCache(const FBoneContainer& BoneContainer) final;
	virtual void InitializeBoneReferences(const FBoneContainer& RequiredBones) final;
};