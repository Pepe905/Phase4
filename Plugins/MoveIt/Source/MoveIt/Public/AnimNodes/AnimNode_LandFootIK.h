// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.

#pragma once

#include "AnimNodes/AnimNode_LocalSkeletalControlBase.h"
#include "AnimNodes/AnimNodes_Shared.h"
#include "Kismet/GameplayStaticsTypes.h"
#include "AnimNode_LandFootIK.generated.h"

class ACharacter;

USTRUCT(BlueprintType)
struct FMIFoot_LandFootIK
{
	GENERATED_BODY()

	FMIFoot_LandFootIK()
		: Bone(FBoneReference())
		, IKBone(FBoneReference())
		, InterpRate(10.f)
		, ReachPct(1.f)
		, bIsLeftFoot(false)
		, BoneIndex(-1)
		, IKBoneIndex(-1)
		, CachedTM(FTransform::Identity)
		, CachedWorldTM(FTransform::Identity)
		, LandTargetTM(FTransform::Identity)
		, FootComponentOffset(FVector::ZeroVector)
		, FootLocation(FVector::ZeroVector)
		, bReinitTarget(true)
		, BlendAlpha(1.f)
		, bBlendIn(true)
	{}

	FMIFoot_LandFootIK(const FBoneReference& InBone, const FBoneReference& InIKBone)
		: Bone(InBone)
		, IKBone(InIKBone)
		, InterpRate(10.f)
		, ReachPct(1.f)
		, bIsLeftFoot(false)
		, BoneIndex(-1)
		, IKBoneIndex(-1)
		, CachedTM(FTransform::Identity)
		, CachedWorldTM(FTransform::Identity)
		, LandTargetTM(FTransform::Identity)
		, FootComponentOffset(FVector::ZeroVector)
		, FootLocation(FVector::ZeroVector)
		, bReinitTarget(true)
		, BlendAlpha(1.f)
		, bBlendIn(true)
	{}


	UPROPERTY(EditAnywhere, Category = Land)
	FBoneReference Bone;

	UPROPERTY(EditAnywhere, Category = Land)
	FBoneReference IKBone;

	/** How fast the legs move toward the ground */
	UPROPERTY(EditAnywhere, Category = Land)
	float InterpRate;

	/** How far towards the ground the legs go */
	UPROPERTY(EditAnywhere, Category = Land, meta = (ClampMax = "1", UIMax = "1"))
	float ReachPct;

	bool bIsLeftFoot;

	FCompactPoseBoneIndex BoneIndex;
	FCompactPoseBoneIndex IKBoneIndex;

	FTransform CachedTM;
	FTransform CachedWorldTM;
	FTransform LandTargetTM;
	FTransform LandCurrentTM;

	FVector FootComponentOffset;

	FVector FootLocation;

	bool bReinitTarget;

	float BlendAlpha;
	bool bBlendIn;

	FORCEINLINE bool operator==(const FMIFoot_LandFootIK& Other) const
	{
		return Bone.BoneName != NAME_None && Bone.BoneName == Other.Bone.BoneName;
	}

	FORCEINLINE bool operator!=(const FMIFoot_LandFootIK& Other) const
	{
		return !(*this == Other);
	}
};

/**
 *	Plants the feet at the predicted landing location
 */
USTRUCT(BlueprintInternalUseOnly)
struct MOVEIT_API FAnimNode_LandFootIK : public FAnimNode_LocalSkeletalControlBase
{
	GENERATED_BODY()

public:
	/** Predicted landing result calculated by MIAnimInstance */
	UPROPERTY(EditAnywhere, Category = Land, meta = (PinShownByDefault))
	FPredictProjectilePathResult PredictedLanding;

	/** Z Velocity must be less than this amount while in the air to begin landing */
	UPROPERTY(EditAnywhere, Category = Land)
	float JumpStartZVelocity;

	/** Must be this close to ground to begin landing */
	UPROPERTY(EditAnywhere, Category = Land)
	float StartMinDistFromGround;

	UPROPERTY(EditAnywhere, Category = Land)
	FBoneReference Pelvis;

	UPROPERTY(EditAnywhere, Category = Land)
	TArray<FMIFoot_LandFootIK> LeftFeet;

	UPROPERTY(EditAnywhere, Category = Land)
	TArray<FMIFoot_LandFootIK> RightFeet;
	
	/** If true, will work during PIE which is useful for sequencer */
	UPROPERTY(EditAnywhere, Category = Land, meta = (PinHiddenByDefault))
	bool bWorkOutsidePIE;

protected:
	FCompactPoseBoneIndex PelvisIndex;

	float DeltaTime; 

	FVector OwnerVelocity;

	ACharacter* Character;

	TArray<FMIFoot_LandFootIK> Feet;

public:
	FAnimNode_LandFootIK()
		: JumpStartZVelocity(150.f)
		, StartMinDistFromGround(750.f)
		, Pelvis(FBoneReference("pelvis"))
		, LeftFeet({ FMIFoot_LandFootIK(FBoneReference("foot_l"), FBoneReference("ik_foot_l")) })
		, RightFeet({ FMIFoot_LandFootIK(FBoneReference("foot_r"), FBoneReference("ik_foot_r")) })
		, bWorkOutsidePIE(false)
		, PelvisIndex(-1)
		, DeltaTime(0.f)
		, OwnerVelocity(FVector::ZeroVector)
		, Character(nullptr)
	{
		AlphaInputType = EAnimAlphaInputType::Bool;
		bAlphaBoolEnabled = false;
		AlphaBoolBlend.BlendInTime = 0.3f;
		AlphaBoolBlend.BlendOutTime = 0.3f;
		AlphaBoolBlend.BlendOption = EAlphaBlendOption::Sinusoidal;
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