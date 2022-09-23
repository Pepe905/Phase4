// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.

#pragma once

#include "AnimNodes/AnimNode_SpringBase.h"
#include "BoneContainer.h"
#include "BonePose.h"
#include "Engine/EngineTypes.h"
#include "AnimNode_JumpingCompression.generated.h"

class ACharacter;


/**
 * Applies physics spring to extend the character using acceleration velocity when jumping
 */
USTRUCT(BlueprintInternalUseOnly)
struct MOVEIT_API FAnimNode_JumpingCompression : public FAnimNode_SpringBase
{
	GENERATED_BODY()

public:
	/** Jumping velocity applied to spring */
	UPROPERTY(EditAnywhere, Category = Jump, meta = (PinShownByDefault))
	FVector ImpactVelocity;

	/** Scale the incoming impact velocity */
	UPROPERTY(EditAnywhere, Category = Jump)
	float ImpactVelocityScale;

	/** Should be lower than CharacterMovementComponent JumpZVelocity while being high enough to cut out compression from tiny bumps */
	UPROPERTY(EditAnywhere, Category = Jump)
	float VelocityZThreshold;

	/** Shortest duration for jumping impulse (at 0 velocity) */
	UPROPERTY(EditAnywhere, Category = Jump)
	float MinJumpTime;

	/** Maximum duration for landing impulse (at VelocityForMaxLandTime) */
	UPROPERTY(EditAnywhere, Category = Jump)
	float MaxJumpTime;

	/** Minimum Velocity required to reach MaxLandTime */
	UPROPERTY(EditAnywhere, Category = Jump)
	float VelocityForMaxJumpTime;

	/** 
	 * Max magnitude to actually apply from ImpactVelocity 
	 * If the character jitters when landing, this is too high or your spring isn't customized correctly
	 */
	UPROPERTY(EditAnywhere, Category = Jump)
	float MaxImpactVelocity;

	/** If true, will work during PIE which is useful for sequencer */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Jump, meta = (PinHiddenByDefault))
	bool bWorkOutsidePIE;

protected:
	bool bJumping;

	UPROPERTY()
	ACharacter* Character;

public:
	FTimerHandle DisableTimerHandle;

public:
	FAnimNode_JumpingCompression()
		: ImpactVelocity(FVector::ZeroVector)
		, ImpactVelocityScale(1.f)
		, VelocityZThreshold(200.f)
		, MinJumpTime(0.1f)
		, MaxJumpTime(0.2f)
		, VelocityForMaxJumpTime(1000.f)
		, MaxImpactVelocity(1000.f)
		, bWorkOutsidePIE(false)
		, bJumping(false)
		, Character(nullptr)
	{
		SpringBone = FBoneReference("spine_01");
		MaxDisplacement = 35.f;
		SpringStrength = 0.2f;
		SpringStiffness = 1000.f;
		SpringDamping = 12.f;
		ErrorResetThresh = 64.f;
		bSimulateInComponentSpace = true;
		bLimitDisplacement = true;

		bTranslateX = false;
		bTranslateY = false;

		AlphaInputType = EAnimAlphaInputType::Bool;
		bAlphaBoolEnabled = false;
		AlphaBoolBlend.BlendInTime = 0.1f;
		AlphaBoolBlend.BlendOutTime = 0.4f;
		AlphaBoolBlend.BlendOption = EAlphaBlendOption::Sinusoidal;
	}

	// FAnimNode_LocalSkeletalControlBase interface
	virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) final;
	virtual void PreUpdate(const UAnimInstance* InAnimInstance) final;
	// End of FAnimNode_LocalSkeletalControlBase interface
};
