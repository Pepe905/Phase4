// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.

#pragma once

#include "AnimNodes/AnimNode_LocalSkeletalControlBase.h"
#include "AnimNodes/AnimNodes_Shared.h"
#include "AnimNode_JumpFootIK.generated.h"

class ACharacter;

USTRUCT(BlueprintType)
struct FMIJumpFootIK_Foot
{
	GENERATED_BODY()

	FMIJumpFootIK_Foot()
		: Foot(FBoneReference())
		, IKFoot(FBoneReference())
		, NumBonesInLimb(2)
		, JumpInterpOutRate(2.f)
		, FallInterpOutRate(2.f)
		, GroundInterpOutRate(5.f)
		, FlyInterpOutRate(5.f)
		, SwimInterpOutRate(5.f)
		, DefaultInterpOutRate(5.f)
		, bIsLeftFoot(false)
		, BoneIndex(-1)
		, IKBoneIndex(-1)
		, CachedTM(FTransform::Identity)
		, CachedWorldTM(FTransform::Identity)
		, JumpStartTM(FTransform::Identity)
		, BlendAlpha(1.f)
	{}

	FMIJumpFootIK_Foot(const FBoneReference& InFoot, const FBoneReference& InIKFoot)
		: Foot(InFoot)
		, IKFoot(InIKFoot)
		, NumBonesInLimb(2)
		, JumpInterpOutRate(2.f)
		, FallInterpOutRate(2.f)
		, GroundInterpOutRate(5.f)
		, FlyInterpOutRate(5.f)
		, SwimInterpOutRate(5.f)
		, DefaultInterpOutRate(5.f)
		, bIsLeftFoot(false)
		, BoneIndex(-1)
		, IKBoneIndex(-1)
		, CachedTM(FTransform::Identity)
		, CachedWorldTM(FTransform::Identity)
		, JumpStartTM(FTransform::Identity)
		, BlendAlpha(1.f)
	{}

	/** Foot bone */
	UPROPERTY(EditAnywhere, Category = Jump)
	FBoneReference Foot;

	/** Foot IK bone */
	UPROPERTY(EditAnywhere, Category = Jump)
	FBoneReference IKFoot;

	/** How many other bones in the same limb (for bipeds/humans, this is 2 for the thigh and calf) */
	UPROPERTY(EditAnywhere, Category = Jump)
	uint8 NumBonesInLimb;

	/** Rate to interpolate out the foot reaching to the ground while jumping upward */
	UPROPERTY(EditAnywhere, Category = Jump)
	float JumpInterpOutRate;

	/** Rate to interpolate out the foot reaching to the ground while falling downward */
	UPROPERTY(EditAnywhere, Category = Jump)
	float FallInterpOutRate;

	/** Rate to interpolate out the foot reaching to the ground while on the ground */
	UPROPERTY(EditAnywhere, Category = Jump)
	float GroundInterpOutRate;

	/** Rate to interpolate out the foot reaching to the ground while flying */
	UPROPERTY(EditAnywhere, Category = Jump)
	float FlyInterpOutRate;

	/** Rate to interpolate out the foot reaching to the ground while swimming */
	UPROPERTY(EditAnywhere, Category = Jump)
	float SwimInterpOutRate;

	/** For when MovementMode is disabled or custom movement mode is used */
	UPROPERTY(EditAnywhere, Category = Jump)
	float DefaultInterpOutRate;

	bool bIsLeftFoot;

	FCompactPoseBoneIndex BoneIndex;
	FCompactPoseBoneIndex IKBoneIndex;

	TArray<FCompactPoseBoneIndex> FKBoneIndices;

	FTransform CachedTM;
	FTransform CachedWorldTM;
	FTransform JumpStartTM;

	float BlendAlpha;

	FORCEINLINE bool operator==(const FMIJumpFootIK_Foot& Other) const
	{
		return Foot.BoneName != NAME_None && Foot.BoneName == Other.Foot.BoneName;
	}

	FORCEINLINE bool operator!=(const FMIJumpFootIK_Foot& Other) const
	{
		return !(*this == Other);
	}
};

/**
 *	Compute best leg to jump off then moves it toward the ground
 */
USTRUCT(BlueprintInternalUseOnly)
struct MOVEIT_API FAnimNode_JumpFootIK : public FAnimNode_LocalSkeletalControlBase
{
	GENERATED_BODY()

public:
	/** If true, jump off the rear foot instead of front foot */
	UPROPERTY(EditAnywhere, Category = Jump)
	bool bUseRearFoot;

	/** Add every left foot that will be used for jumping */
	UPROPERTY(EditAnywhere, Category = Jump)
	TArray<FMIJumpFootIK_Foot> LeftFeet;

	/** Add every right foot that will be used for jumping */
	UPROPERTY(EditAnywhere, Category = Jump)
	TArray<FMIJumpFootIK_Foot> RightFeet;

protected:
	float DeltaTime; 

	FVector OwnerVelocity;

	ACharacter* Character;

	bool bWasFalling;

	EMIJumpState State;

	TArray<FMIJumpFootIK_Foot> Feet;

	FMIJumpFootIK_Foot* JumpingFoot;

public:
	FAnimNode_JumpFootIK()
		: bUseRearFoot(true)
		, LeftFeet( { FMIJumpFootIK_Foot(FBoneReference("foot_l"), FBoneReference("ik_foot_l")) })
		, RightFeet({ FMIJumpFootIK_Foot(FBoneReference("foot_r"), FBoneReference("ik_foot_r")) })
		, DeltaTime(0.f)
		, OwnerVelocity(FVector::ZeroVector)
		, Character(nullptr)
		, bWasFalling(false)
		, State(EMIJumpState::JS_Ground)
		, JumpingFoot(nullptr)
	{}

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