// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.

#pragma once

#include "BoneControllers/AnimNode_SkeletalControlBase.h"
#include "AnimNode_FootIK.generated.h"

USTRUCT(BlueprintType)
struct FMIFootIK_State
{
	GENERATED_BODY()

	FMIFootIK_State()
		: OrientationRate(15.f)
		, LiftFootRate(10.f)
		, LowerFootRate(15.f)
		, LiftMaxZLimit(20.f)
		, LowerMaxZLimit(-50.f)
		, LowerMaxZLimitOverEdge(-20.f)
		, PitchDownLimit(20.f)
		, PitchUpLimit(-35.f)
		, RollLeftLimit(15.f)
		, RollRightLimit(-15.f)
		, ZOffset(0.f)
	{}

	/** How fast the foot rotates to orient to the ground */
	UPROPERTY(EditAnywhere, Category = IK)
	float OrientationRate;

	/** How fast the foot moves up */
	UPROPERTY(EditAnywhere, Category = IK)
	float LiftFootRate;

	/** How fast the foot moves down */
	UPROPERTY(EditAnywhere, Category = IK)
	float LowerFootRate;

	/** How high the foot can go */
	UPROPERTY(EditAnywhere, Category = IK)
	float LiftMaxZLimit;

	/** How low the foot can go */
	UPROPERTY(EditAnywhere, Category = IK)
	float LowerMaxZLimit;

	/** How low the foot can go if its over the edge (not touching ground) */
	UPROPERTY(EditAnywhere, Category = IK)
	float LowerMaxZLimitOverEdge;

	/** How far the foot can rotate down on pitch */
	UPROPERTY(EditAnywhere, Category = IK)
	float PitchDownLimit;

	/** How far the foot can rotate up on pitch */
	UPROPERTY(EditAnywhere, Category = IK)
	float PitchUpLimit;

	/** How far the foot can rotate left on roll */
	UPROPERTY(EditAnywhere, Category = IK)
	float RollLeftLimit;

	/** How far the foot can rotate right on roll */
	UPROPERTY(EditAnywhere, Category = IK)
	float RollRightLimit;

	/** Offset the final position on Z */
	UPROPERTY(EditAnywhere, Category = IK)
	float ZOffset;
};

USTRUCT(BlueprintType)
struct FMIFootIK_Foot
{
	GENERATED_BODY()

	FMIFootIK_Foot()
		: bUseBoxForTrace(false)
		, FootSize(FVector(0.001f, 13.f, 6.f))
		, FootBoxCenterOffset(FVector::ZeroVector)
		, Target(FVector::ZeroVector)
		, Translation(FVector::ZeroVector)
		, PelvisTarget(FVector2D::ZeroVector)
		, OrientTarget(FRotator::ZeroRotator)
		, Orientation(FRotator::ZeroRotator)
		, bOverEdge(false)
		, TranslateInterpRate(0.f)
		, bIsLeftFoot(false)
		, CachedWorldLoc(FVector::ZeroVector)
		, CachedWorldBaseLoc(FVector::ZeroVector)
		, CachedBoneLoc(FVector::ZeroVector)
		, BoneIndex(-1)
		, IKBoneIndex(-1)
	{
		ConstructStates();
	}

	FMIFootIK_Foot(const FBoneReference& InBone, const FBoneReference& InIKBone)
		: Bone(InBone)
		, IKBone(InIKBone)
		, bUseBoxForTrace(false)
		, FootSize(FVector(0.001f, 13.f, 6.f))
		, FootBoxCenterOffset(FVector::ZeroVector)
		, Target(FVector::ZeroVector)
		, Translation(FVector::ZeroVector)
		, PelvisTarget(FVector2D::ZeroVector)
		, OrientTarget(FRotator::ZeroRotator)
		, Orientation(FRotator::ZeroRotator)
		, bOverEdge(false)
		, TranslateInterpRate(0.f)
		, bIsLeftFoot(false)
		, CachedWorldLoc(FVector::ZeroVector)
		, CachedWorldBaseLoc(FVector::ZeroVector)
		, CachedBoneLoc(FVector::ZeroVector)
		, BoneIndex(-1)
		, IKBoneIndex(-1)
	{
		ConstructStates();
	}

	void ConstructStates()
	{
		States.Init(FMIFootIK_State(), 3);

		FMIFootIK_State& Crouch = States[1];

		Crouch.OrientationRate = 15.f;
		Crouch.LiftFootRate = 10.f;
		Crouch.LowerFootRate = 15.f;
		Crouch.LiftMaxZLimit = 10.f;
		Crouch.LowerMaxZLimit = -15.f;
		Crouch.LowerMaxZLimitOverEdge = -15.f;
		Crouch.PitchDownLimit = 10.f;
		Crouch.PitchUpLimit = -15.f;
		Crouch.RollLeftLimit = 5.f;
		Crouch.RollRightLimit = -5.f;
		Crouch.ZOffset = 0.f;

		FMIFootIK_State& FloorSlide = States[2];

		FloorSlide.OrientationRate = 15.f;
		FloorSlide.LiftFootRate = 10.f;
		FloorSlide.LowerFootRate = 15.f;
		FloorSlide.LiftMaxZLimit = 20.f;
		FloorSlide.LowerMaxZLimit = -50.f;
		FloorSlide.LowerMaxZLimitOverEdge = -20.f;
		FloorSlide.PitchDownLimit = 0.f;
		FloorSlide.PitchUpLimit = 0.f;
		FloorSlide.RollLeftLimit = 0.f;
		FloorSlide.RollRightLimit = 0.f;
		FloorSlide.ZOffset = 0.f;
	}

	/** Foot bone */
	UPROPERTY(EditAnywhere, Category = IK)
	FBoneReference Bone;

	/** Foot IK bone */
	UPROPERTY(EditAnywhere, Category = IK)
	FBoneReference IKBone;

	//UPROPERTY(EditAnywhere, Category = IK)
	bool bUseBoxForTrace;

	/** Size of this foot; recommend enable debug drawing. For standard UE4 skeleton this is Height, Length, Width */
	//UPROPERTY(EditAnywhere, Category = IK)
	FVector FootSize;

	/** Used to move the center of the foot */
	//UPROPERTY(EditAnywhere, Category = IK)
	FVector FootBoxCenterOffset;

	/**
	 * Optional states to allow for different settings when crouching, sprinting, etc
	 * Must always have at least one entry, or will abort applying IK
	 */
	UPROPERTY(EditAnywhere, Category = IK)
	TArray<FMIFootIK_State> States;

	/** Desired foot position */
	FVector Target;

	/** Resulting foot translation, interpolating towards Target */
	FVector Translation;

	/** Desired pelvis position based on this foot's Target */
	FVector2D PelvisTarget;

	/** Desired foot orientation */
	FRotator OrientTarget;

	/** Resulting foot orientation, interpolating towards OrientTarget */
	FRotator Orientation;

	/** True if this foot is over the edge (hanging in air) */
	bool bOverEdge;

	/** Resulting interp rate based on the direction the foot is moving on Z */
	float TranslateInterpRate;

	/** 
	 * Used to differentiate between feet on both sides
	 * NOT CURRENTLY IN USE
	 */
	bool bIsLeftFoot;

	/** MeshTM.Rotator().RotateVector(BoneTM.GetLocation()) + MeshLoc */
	FVector CachedWorldLoc;

	/** OrientRotation.UnrotateVector(Foot.CachedWorldLoc - MeshLoc) + MeshLoc */
	FVector CachedWorldBaseLoc;

	FVector CachedBoneLoc;

	/** 
	 * Foot.CachedWorldLoc - Foot.CachedWorldBaseLoc
	 * This is the difference between where the foot is now and where it will be after
	 * the mesh is rotated for orienting (rotating the mesh after IK will otherwise push 
	 * the feet into the floor)
	 */
	FVector CachedFloorOrientOffset;

	FCompactPoseBoneIndex BoneIndex;
	FCompactPoseBoneIndex IKBoneIndex;
};

/**
 *	Computes and applies advanced IK for each foot, shifts the weight of the pelvis, handles orientation to floor
 */
USTRUCT(BlueprintInternalUseOnly)
struct MOVEIT_API FAnimNode_FootIK : public FAnimNode_SkeletalControlBase
{
	GENERATED_BODY()

public:
	/** Character root bone, this is usually the first bone in the skeletal hierarchy */
	UPROPERTY(EditAnywhere, Category = IK)
	FBoneReference Root;

	/** Character pelvis bone, usually immediately beneath root (also called: hips, cog) */
	UPROPERTY(EditAnywhere, Category = IK)
	FBoneReference Pelvis;

	/** Add every left foot that will be modified */
	UPROPERTY(EditAnywhere, Category = IK)
	TArray<FMIFootIK_Foot> LeftFeet;

	/** Add every right foot that will be modified */
	UPROPERTY(EditAnywhere, Category = IK)
	TArray<FMIFootIK_Foot> RightFeet;

	/** Same translation applied to the pelvis also applied to these joints (mainly for keeping hand ik moving along with pelvis) */
	UPROPERTY(EditAnywhere, Category = IK)
	TArray<FBoneReference> FollowPelvis;

	/** The state the character is currently in - Index to Feet::States, determining which to use */
	UPROPERTY(EditAnywhere, Category = IK, meta = (PinShownByDefault))
	uint8 State;

	/** How fast the pelvis adjusts its weight when both feet are in the air */
	UPROPERTY(EditAnywhere, Category = IK)
	float PelvisXYTranslateRate;

	/** How far above the mesh to trace from */
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = "IK Config")
	float TraceStartHeightAboveMesh;

	/** How far below the mesh to trace to */
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = "IK Config")
	float TraceLengthBelowMesh;

	/** Channel to trace for foot IK */
	UPROPERTY(EditAnywhere, Category = "IK Config")
	TEnumAsByte<ECollisionChannel> FootTraceChannel;

	/** If true, will work during PIE which is useful for sequencer */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IK Config", meta = (PinHiddenByDefault))
	bool bWorkOutsidePIE;

	/** If true will draw debug helpers when playing in editor */
	UPROPERTY(EditAnywhere, Category = Debug)
	bool bEnableDebugDrawPIE;

protected:
	FORCEINLINE ETraceTypeQuery FootTraceTypeQuery() const
	{
		return UEngineTypes::ConvertToTraceType(FootTraceChannel);
	}

	TArray<FMIFootIK_Foot> Feet;

	/** World-space location of the bone. */
	FVector BoneLocation;

	/** World-space velocity of the bone. */
	FVector BoneVelocity;

	/** Velocity of the owning actor */
	FVector OwnerVelocity;

	float DeltaTime;

	FRotator OrientRotation;
	FVector PelvisTranslation;

	FCompactPoseBoneIndex RootBoneIndex;
	FCompactPoseBoneIndex PelvisBoneIndex;
	TArray<FCompactPoseBoneIndex> FollowPelvisIndices;

	float RootYawOffset;

	bool bValidProxy;

public:
	FAnimNode_FootIK()
		: Root(FBoneReference("root"))
		, Pelvis(FBoneReference("pelvis"))
		, LeftFeet(
			{
				FMIFootIK_Foot(FBoneReference("foot_l"), FBoneReference("ik_foot_l"))
			})
		, RightFeet(
			{
				FMIFootIK_Foot(FBoneReference("foot_r"), FBoneReference("ik_foot_r"))
			})
		, FollowPelvis({ FBoneReference("ik_hand_root") })
		, State(0)
		, PelvisXYTranslateRate(8.f)
		, TraceStartHeightAboveMesh(50.f)
		, TraceLengthBelowMesh(75.f)
		, FootTraceChannel(ECC_Visibility)
		, bWorkOutsidePIE(false)
		, bEnableDebugDrawPIE(false)
		, BoneLocation(FVector::ZeroVector)
		, BoneVelocity(FVector::ZeroVector)
		, OwnerVelocity(FVector::ZeroVector)
		, DeltaTime(0.f)
		, OrientRotation(FRotator::ZeroRotator)
		, PelvisTranslation(FVector::ZeroVector)
		, RootBoneIndex(-1)
		, PelvisBoneIndex(-1)
		, RootYawOffset(0.f)
		, bValidProxy(false)
	{
		AlphaInputType = EAnimAlphaInputType::Bool;
		AlphaBoolBlend.BlendInTime = 0.2f;
		AlphaBoolBlend.BlendOutTime = 0.2f;
		AlphaBoolBlend.BlendOption = EAlphaBlendOption::HermiteCubic;
	}

	// FAnimNode_Base interface
	virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) final;
	virtual void UpdateInternal(const FAnimationUpdateContext& Context) final;
	virtual bool HasPreUpdate() const final { return true; }
	virtual void PreUpdate(const UAnimInstance* InAnimInstance) final;
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

public:
#if WITH_EDITOR
	void ConditionalDebugDraw(FPrimitiveDrawInterface* PDI, USkeletalMeshComponent* MeshComp) const;
#endif // WITH_EDITOR
};