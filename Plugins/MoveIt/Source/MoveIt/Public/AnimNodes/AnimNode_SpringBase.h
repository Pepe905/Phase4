// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.

#pragma once

#include "AnimNodes/AnimNode_LocalSkeletalControlBase.h"
#include "BoneContainer.h"
#include "BonePose.h"
#include "AnimNode_SpringBase.generated.h"

class UAnimInstance;
class USkeletalMeshComponent;

USTRUCT(BlueprintType)
struct FAnimLimits
{
	GENERATED_BODY()

	FAnimLimits()
		: bX(false)
		, bY(false)
		, bZ(false)
	{}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings)
	bool bX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings)
	bool bY;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings)
	bool bZ;
};

/**
 *	Simple controller that replaces or adds to the translation/rotation of a single bone.
 *  With added ability to limit individual translation axis
 */
USTRUCT(BlueprintInternalUseOnly)
struct MOVEIT_API FAnimNode_SpringBase : public FAnimNode_LocalSkeletalControlBase
{
	GENERATED_BODY()

public:
	/** External velocity applied to spring (component space simulation only) */
	UPROPERTY(EditAnywhere, Category = Spring, meta = (PinHiddenByDefault))
	FVector Velocity;

	/** Name of bone to control. This is the main bone chain to modify from. **/
	UPROPERTY(EditAnywhere, Category=Spring) 
	FBoneReference SpringBone;

	/** If bLimitDisplacement is true, this indicates how long a bone can stretch beyond its length in the ref-pose. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Spring, meta=(EditCondition="bLimitDisplacement"))
	float MaxDisplacement;

	UPROPERTY(EditAnywhere, Category = Spring, meta = (PinHiddenByDefault))
	float SpringStrength;

	/** Stiffness of spring */
	UPROPERTY(EditAnywhere, Category=Spring, meta = (PinHiddenByDefault))
	float SpringStiffness;

	/** Damping of spring */
	UPROPERTY(EditAnywhere, Category=Spring, meta = (PinHiddenByDefault))
	float SpringDamping;

	/** If spring stretches more than this, reset it. Useful for catching teleports etc */
	UPROPERTY(EditAnywhere, Category=Spring)
	float ErrorResetThresh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = Spring)
	float SimulationFrequency;

	UPROPERTY(EditAnywhere, Category = Spring)
	bool bSimulateInComponentSpace;

	UPROPERTY(EditAnywhere, Category = Spring)
	FAnimLimits LimitMinTranslation;

	UPROPERTY(EditAnywhere, Category = Spring)
	FAnimLimits LimitMaxTranslation;

	UPROPERTY(EditAnywhere, Category = Spring)
	FVector MinTranslationLimits;

	UPROPERTY(EditAnywhere, Category = Spring)
	FVector MaxTranslationLimits;

	/** Limit the amount that a bone can stretch from its ref-pose length. */
	UPROPERTY(EditAnywhere, Category=Spring)
	uint8 bLimitDisplacement : 1;

	/** If true take the spring calculation for translation in X */
	UPROPERTY(EditAnywhere, Category=FilterChannels)
	uint8 bTranslateX : 1;

	/** If true take the spring calculation for translation in Y */
	UPROPERTY(EditAnywhere, Category=FilterChannels)
	uint8 bTranslateY : 1;

	/** If true take the spring calculation for translation in Z */
	UPROPERTY(EditAnywhere, Category=FilterChannels)
	uint8 bTranslateZ : 1;

	/** If true take the spring calculation for rotation in X */
	UPROPERTY(EditAnywhere, Category=FilterChannels)
	uint8 bRotateX : 1;

	/** If true take the spring calculation for rotation in Y */
	UPROPERTY(EditAnywhere, Category=FilterChannels)
	uint8 bRotateY : 1;

	/** If true take the spring calculation for rotation in Z */
	UPROPERTY(EditAnywhere, Category=FilterChannels)
	uint8 bRotateZ : 1;

protected:
	/** Did we have a non-zero ControlStrength last frame. */
	uint8 bHadValidStrength : 1;

	/** World-space location of the bone. */
	FVector BoneLocation;

	/** World-space velocity of the bone. */
	FVector BoneVelocity;

	/** Velocity of the owning actor */
	FVector OwnerVelocity;

	/** Cache of previous frames local bone transform so that when
	 *  we cannot simulate (timestep == 0) we can still update bone location */
	FVector LocalBoneTransform;

	/** Internal use - Amount of time we need to simulate. */
	float RemainingTime;

	/** Internal use - Current timestep */
	float FixedTimeStep;

	/** Internal use - Current time dilation */
	float TimeDilation;

public:
	FAnimNode_SpringBase()
		: Velocity(FVector::ZeroVector)
		, MaxDisplacement(0.0f)
		, SpringStrength(1.f)
		, SpringStiffness(50.0f)
		, SpringDamping(4.0f)
		, ErrorResetThresh(256.0f)
		, SimulationFrequency(120.f)
		, bSimulateInComponentSpace(false)
		, MinTranslationLimits(FVector::ZeroVector)
		, MaxTranslationLimits(FVector::ZeroVector)
		, bLimitDisplacement(false)
		, bTranslateX(true)
		, bTranslateY(true)
		, bTranslateZ(true)
		, bRotateX(false)
		, bRotateY(false)
		, bRotateZ(false)
		, bHadValidStrength(false)
		, BoneLocation(FVector::ZeroVector)
		, BoneVelocity(FVector::ZeroVector)
		, OwnerVelocity(FVector::ZeroVector)
		, RemainingTime(0.f)
		, FixedTimeStep(0)
		, TimeDilation(0)
	{
	}

	// FAnimNode_Base interface
	virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
	virtual bool HasPreUpdate() const override final { return true; }
	virtual void PreUpdate(const UAnimInstance* InAnimInstance) override;
	// End of FAnimNode_Base interface

	// FAnimNode_LocalSkeletalControlBase interface
	virtual void UpdateInternal(const FAnimationUpdateContext& Context) override final;
	virtual void EvaluateLocalSkeletalControl_AnyThread(FPoseContext& Output, float BlendWeight) override;
	virtual bool IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) override final;
	// End of FAnimNode_LocalSkeletalControlBase interface

	virtual void InitializeBoneParentCache(const FBoneContainer& BoneContainer) override final;
	virtual void InitializeBoneReferences(const FBoneContainer& RequiredBones) override final;

protected:
	FORCEINLINE static void LimitDisplacement(double& BoneLocation, const float TargetPos, const bool bLimitMinTranslation, const bool bLimitMaxTranslation, const float MinTranslationLimit, const float MaxTranslationLimit)
	{
		const float DiffSq = (BoneLocation - TargetPos) * (BoneLocation - TargetPos);
		if (bLimitMaxTranslation && BoneLocation > TargetPos)
		{
			if (BoneLocation != TargetPos && DiffSq > FMath::Square(MaxTranslationLimit))
			{
				BoneLocation = 0.0;
			}
		}
		if (bLimitMinTranslation && BoneLocation < TargetPos)
		{
			if (BoneLocation != TargetPos && DiffSq > FMath::Square(MinTranslationLimit))
			{
				BoneLocation = TargetPos + (MinTranslationLimit);
			}
		}
	}

};
