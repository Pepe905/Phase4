// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.

#pragma once

#include "AnimNodes/AnimNode_AdditiveBlendSpace.h"
#include "Kismet/GameplayStaticsTypes.h"
#include "AnimNode_LandingPose.generated.h"

class ACharacter;
struct FPredictProjectilePathResult;


/**
 *	Predicts where the character will land and adapts pose based on distance to ground and movement direction
 */
USTRUCT(BlueprintInternalUseOnly)
struct MOVEIT_API FAnimNode_LandingPose : public FAnimNode_AdditiveBlendSpace
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

	/** How fast the lateral velocity interpolates going up - 0 for no interpolation */
	UPROPERTY(EditAnywhere, Category = Land)
	float LateralInterpUpRate;

	/** How fast the lateral velocity interpolates going down (to neutral pose) - 0 for no interpolation */
	UPROPERTY(EditAnywhere, Category = Land)
	float LateralInterpDownRate;

	/** If true, will work during PIE which is useful for sequencer */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Land, meta = (PinHiddenByDefault))
	bool bWorkOutsidePIE;

protected:
	ACharacter* Character;

	FVector OwnerVelocity;

	bool bEnableLanding;

public:
	FAnimNode_LandingPose()
		: JumpStartZVelocity(300.f)
		, StartMinDistFromGround(750.f)
		, LateralInterpUpRate(50.f)
		, LateralInterpDownRate(10.f)
		, bWorkOutsidePIE(false)
		, Character(nullptr)
		, OwnerVelocity(FVector::ZeroVector)
		, bEnableLanding(false)
	{
		AlphaInputType = EAnimAlphaInputType::Bool;
		AlphaBoolBlend.BlendInTime = 0.1f;
		AlphaBoolBlend.BlendOutTime = 0.1f;
		AlphaBoolBlend.BlendOption = EAlphaBlendOption::HermiteCubic;

		AdditiveType = EMIAdditiveType::MIAS_MeshSpaceAdditive;
	}

protected:
	virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) final;
	virtual void PreUpdate(const UAnimInstance* InAnimInstance) final;
	virtual bool HasPreUpdate() const final { return true; }
	virtual void UpdateBlendSpace(const FAnimationUpdateContext& Context) final;

	bool PredictLandingLocation(FPredictProjectilePathResult& PredictResult);
};