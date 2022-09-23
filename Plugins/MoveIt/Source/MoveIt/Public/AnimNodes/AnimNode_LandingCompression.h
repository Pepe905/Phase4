// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.

#pragma once

#include "AnimNodes/AnimNode_SpringBase.h"
#include "BoneContainer.h"
#include "BonePose.h"
#include "Engine/EngineTypes.h"
#include "AnimNode_LandingCompression.generated.h"

class ACharacter;


/**
 * Applies physics spring to compress the character using impact velocity when landing
 */
USTRUCT(BlueprintInternalUseOnly)
struct MOVEIT_API FAnimNode_LandingCompression : public FAnimNode_SpringBase
{
	GENERATED_BODY()

public:
	/** Landing velocity applied to spring */
	UPROPERTY(EditAnywhere, Category = Landing, meta = (PinShownByDefault))
	FVector ImpactVelocity;

	/** Scale the incoming impact velocity */
	UPROPERTY(EditAnywhere, Category = Landing)
	float ImpactVelocityScale;

	UPROPERTY(EditAnywhere, Category = Landing)
	float VelocityZThreshold;

	/** Shortest duration for landing impact (at 0 velocity) */
	UPROPERTY(EditAnywhere, Category = Landing)
	float MinLandTime;

	/** Maximum duration for landing impact (at VelocityForMaxLandTime) */
	UPROPERTY(EditAnywhere, Category = Landing)
	float MaxLandTime;

	/** Minimum Velocity required to reach MaxLandTime */
	UPROPERTY(EditAnywhere, Category = Landing)
	float VelocityForMaxLandTime;

	/** 
	 * Max magnitude to actually apply from ImpactVelocity 
	 * If the character jitters when landing, this is too high or your spring isn't customized correctly
	 */
	UPROPERTY(EditAnywhere, Category = Landing)
	float MaxImpactVelocity;

	/** If true, will work during PIE which is useful for sequencer */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Landing, meta = (PinHiddenByDefault))
	bool bWorkOutsidePIE;

protected:
	bool bFalling;

	UPROPERTY()
	ACharacter* Character;

public:
	FTimerHandle DisableTimerHandle;

public:
	FAnimNode_LandingCompression()
		: ImpactVelocity(FVector::ZeroVector)
		, ImpactVelocityScale(1.f)
		, VelocityZThreshold(10.f)
		, MinLandTime(0.05f)
		, MaxLandTime(0.1f)
		, VelocityForMaxLandTime(2600.f)
		, MaxImpactVelocity(3000.f)
		, bWorkOutsidePIE(false)
		, bFalling(false)
		, Character(nullptr)
	{
		SpringBone = FBoneReference("pelvis");
		MaxDisplacement = 35.f;
		SpringStrength = 1.f;
		SpringStiffness = 250.f;
		SpringDamping = 10.f;
		ErrorResetThresh = 64.f;
		bSimulateInComponentSpace = true;
		bLimitDisplacement = true;

		bTranslateX = false;
		bTranslateY = false;

		AlphaInputType = EAnimAlphaInputType::Bool;
		bAlphaBoolEnabled = false;
		AlphaBoolBlend.BlendInTime = 0.1f;
		AlphaBoolBlend.BlendOutTime = 0.2f;
		AlphaBoolBlend.BlendOption = EAlphaBlendOption::Sinusoidal;
	}

	// FAnimNode_LocalSkeletalControlBase interface
	virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) final;
	virtual void PreUpdate(const UAnimInstance* InAnimInstance) final;
	// End of FAnimNode_LocalSkeletalControlBase interface
};
