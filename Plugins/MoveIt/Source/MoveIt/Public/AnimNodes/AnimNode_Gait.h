// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.

#pragma once

#include "AnimNodes/AnimNode_LocalSkeletalControlBase.h"
#include "AnimNodes/AnimNodes_Shared.h"
#include "Curves/CurveFloat.h"
#include "AnimNode_Gait.generated.h"


USTRUCT(BlueprintType)
struct FMIGait_Foot
{
	GENERATED_BODY()

	FMIGait_Foot()
		: LimbRootLocation(FVector::ZeroVector)
		, OriginLocation(FVector::ZeroVector)
		, ActualLocation(FVector::ZeroVector)
		, BoneLocation(FVector::ZeroVector)
		, BoneVelocity(FVector::ZeroVector)
	{}

	FMIGait_Foot(const FBoneReference& InFoot, const FBoneReference& InIKFoot, const FBoneReference& InPole, const FBoneReference& InIKPole, const FBoneReference& InParent)
		: Foot(InFoot)
		, IKFoot(InIKFoot)
		, Pole(InPole)
		, IKPole(InIKPole)
		, Parent(InParent)
		, LimbRootLocation(FVector::ZeroVector)
		, OriginLocation(FVector::ZeroVector)
		, ActualLocation(FVector::ZeroVector)
		, BoneLocation(FVector::ZeroVector)
		, BoneVelocity(FVector::ZeroVector)
	{}

	UPROPERTY(EditAnywhere, Category = Gait)
	FBoneReference Foot;

	UPROPERTY(EditAnywhere, Category = Gait)
	FBoneReference IKFoot;

	/** This is usually the knee/calf */
	UPROPERTY(EditAnywhere, Category = Gait)
	FBoneReference Pole;

	/** This is usually the knee/calf */
	UPROPERTY(EditAnywhere, Category = Gait)
	FBoneReference IKPole;

	/** This is usually the thigh */
	UPROPERTY(EditAnywhere, Category = Gait)
	FBoneReference Parent;

	UPROPERTY(Transient)
	FVector LimbRootLocation;

	UPROPERTY(Transient)
	FVector OriginLocation;

	UPROPERTY(Transient)
	FVector ActualLocation;

	UPROPERTY(Transient)
	FVector BoneLocation;

	UPROPERTY(Transient)
	FVector BoneVelocity;

	FORCEINLINE bool operator==(const FMIGait_Foot& Other) const
	{
		// Don't bother comparing everything, no two unique bones should be in different structs
		return Foot.HasValidSetup() && Foot == Other.Foot 
			&& IKFoot.HasValidSetup() && IKFoot == Other.IKFoot;
	}
};

/**
 *	Changes the distance between the feet while moving, and applies a spring to the pelvis to modify the weight at shorter gaits
 *	Using this node allows characters to walk into walls without the legs clipping, as it will ultimately run on the spot instead
 *	Additionally gives the benefit of start and stop animations, procedurally
 */
USTRUCT(BlueprintInternalUseOnly)
struct MOVEIT_API FAnimNode_Gait : public FAnimNode_LocalSkeletalControlBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = Gait)
	FBoneReference Pelvis;

	/** The bone above the feet IK bones (they should have an IK root) */
	UPROPERTY(EditAnywhere, Category = Gait)
	FBoneReference IKRoot;

	/** Add each foot that will be modified */
	UPROPERTY(EditAnywhere, Category = Gait)
	TArray<FMIGait_Foot> Feet;

	/** 0->1 range, usually normalized character speed */
	UPROPERTY(EditAnywhere, Category = Gait, meta = (PinShownByDefault))
	float Gait;

	/** Custom multiplier that can be helpful to modify gait based on state */
	UPROPERTY(EditAnywhere, Category = Gait, meta = (PinHiddenByDefault))
	float GaitMultiplier;

	/** Used to prevent feet being too close together when starting to move */
	UPROPERTY(EditAnywhere, Category = Gait, meta = (PinHiddenByDefault))
	float MinGait;

	/** Used to prevent excessive gait */
	UPROPERTY(EditAnywhere, Category = Gait, meta = (PinHiddenByDefault))
	float MaxGait;

	/** Will adjust based on slope of surface walked on (less gait when walking uphill) */
	UPROPERTY(EditAnywhere, Category = Gait)
	bool bAdjustGaitToSlope;

	/** Modify gait based on the slope of the surface */
	UPROPERTY(EditAnywhere, Category = Gait, meta = (EditCondition="bAdjustGaitToSlope"))
	FRuntimeFloatCurve SlopeAngleGaitMultiplierCurve;

	/** How fast the slope change updates - set to 0 for no smoothing */
	UPROPERTY(EditAnywhere, Category = Gait)
	float SlopeSmoothingRate;

	/** How far ahead to trace when predicting the impending slope */
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = Gait, meta = (EditCondition = "bAdjustGaitToSlope"))
	float SlopeTraceAheadDist;

	/** How many times to sample tracing ahead when predicting the impending slope */
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = Gait, meta = (EditCondition = "bAdjustGaitToSlope"))
	int32 SlopeTraceAheadTraceCount;

	/** How far above the mesh to trace from */
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = Gait)
	float SlopeTraceStartHeightAboveMesh;

	/** How far below the mesh to trace to */
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = Gait)
	float SlopeTraceLengthBelowMesh;

	UPROPERTY(EditAnywhere, Category = Gait)
	TEnumAsByte<ECollisionChannel> SlopeTraceChannel;

	FORCEINLINE ETraceTypeQuery SlopeTraceType() const
	{
		return UEngineTypes::ConvertToTraceType(SlopeTraceChannel);
	}

	/** Higher values require feet to move farther from ground when starting to move */
	UPROPERTY(EditAnywhere, Category = Gait, meta = (PinHiddenByDefault))
	float MinSpeedFootHeightBias;

	/** Lower values require feet to move closer to ground when moving at max speed */
	UPROPERTY(EditAnywhere, Category = Gait, meta = (PinHiddenByDefault))
	float MaxSpeedFootHeightBias;

	/** Pelvis spring: How much displacement occurs */
	UPROPERTY(EditAnywhere, Category = Gait)
	float PelvisDisplacement;

	/** Pelvis spring: How tight (responsive) the spring is */
	UPROPERTY(EditAnywhere, Category = Gait)
	float PelvisTightness;

	/** Pelvis spring: How much damping to apply (reducing overall effect) */
	UPROPERTY(EditAnywhere, Category = Gait)
	float PelvisDamping;
	
	/** If true, the IK length wont exceed the base bone length */
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = Gait)
	bool bClampIKLengthToBoneLength;

protected:
	/** Internal use - Amount of time we need to simulate. */
	float RemainingTime;

	/** Internal use - Current timestep */
	float FixedTimeStep;

	/** Internal use - Current time dilation */
	float TimeDilation;

	/** Velocity of the owning actor */
	FVector OwnerVelocity;

	float CurrentSlopeGaitMultiplier;

public:
	FAnimNode_Gait()
		: Pelvis(FBoneReference("pelvis"))
		, IKRoot(FBoneReference("ik_foot_root"))
		, Feet({ 
				FMIGait_Foot(FBoneReference("foot_l"), FBoneReference("ik_foot_l"),FBoneReference("calf_l"),FBoneReference("VB thigh_l_calf_l"),FBoneReference("thigh_l")),
				FMIGait_Foot(FBoneReference("foot_r"), FBoneReference("ik_foot_r"),FBoneReference("calf_r"),FBoneReference("VB thigh_r_calf_r"),FBoneReference("thigh_r"))
			})
		, Gait(1.f)
		, GaitMultiplier(1.f)
		, MinGait(0.4f)
		, MaxGait(2.f)
		, bAdjustGaitToSlope(true)
		, SlopeSmoothingRate(1.f)
		, SlopeTraceAheadDist(400.f)
		, SlopeTraceAheadTraceCount(5)
		, SlopeTraceStartHeightAboveMesh(100.f)
		, SlopeTraceLengthBelowMesh(50.f)
		, SlopeTraceChannel(ECC_Visibility)
		, MinSpeedFootHeightBias(.65f)
		, MaxSpeedFootHeightBias(1.f)
		, PelvisDisplacement(4.f)
		, PelvisTightness(5.f)
		, PelvisDamping(200.f)
		, bClampIKLengthToBoneLength(true)
		, RemainingTime(0.f)
		, FixedTimeStep(0.f)
		, TimeDilation(1.f)
		, OwnerVelocity(FVector::ZeroVector)
		, CurrentSlopeGaitMultiplier(1.f)
	{
		SlopeAngleGaitMultiplierCurve.GetRichCurve()->AddKey(25.f, 1.f);
		SlopeAngleGaitMultiplierCurve.GetRichCurve()->AddKey(60.f, 0.2f);

		AlphaInputType = EAnimAlphaInputType::Bool;
		AlphaBoolBlend.BlendInTime = 0.2f;
		AlphaBoolBlend.BlendOutTime = 0.2f;
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