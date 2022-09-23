// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.

#pragma once

#include "AnimNodes/AnimNode_AdditiveBlendSpace.h"
#include "AnimNode_Duck.generated.h"


/**
 *	Ducks under obstacles
 */
USTRUCT(BlueprintInternalUseOnly)
struct MOVEIT_API FAnimNode_Duck : public FAnimNode_AdditiveBlendSpace
{
	GENERATED_BODY()

public:
	/** Controls the ducking relative to the height of objects above character. Intended to be the original capsule half height but may need to play with it to get the results you want. */
	UPROPERTY(EditAnywhere, Category = Duck)
	float HalfHeight;

	/** How far to search ahead for obstacles that we're about to move under based on character's velocity. */
	UPROPERTY(EditAnywhere, Category = Duck)
	float AnticipationDistance;

	/** How fast to apply ducking */
	UPROPERTY(EditAnywhere, Category = Duck)
	float DuckRate;

	/** How fast to remove ducking */
	UPROPERTY(EditAnywhere, Category = Duck)
	float ResetRate;

	/** Channel to trace for foot IK */
	UPROPERTY(EditAnywhere, Category = Duck)
	TEnumAsByte<ECollisionChannel> DuckTraceChannel;

	/** If true, will work during PIE which is useful for sequencer */
	UPROPERTY(EditAnywhere, Category = Duck, meta = (PinHiddenByDefault))
	bool bWorkOutsidePIE;

protected:
	FORCEINLINE ETraceTypeQuery DuckTraceTypeQuery() const
	{
		return UEngineTypes::ConvertToTraceType(DuckTraceChannel);
	}

public:
	FAnimNode_Duck()
		: HalfHeight(86.f)
		, AnticipationDistance(40.f)
		, DuckRate(15.f)
		, ResetRate(2.f)
		, DuckTraceChannel(ECC_WorldStatic)
		, bWorkOutsidePIE(false)
	{
		AlphaInputType = EAnimAlphaInputType::Bool;
		AlphaBoolBlend.BlendInTime = 0.1f;
		AlphaBoolBlend.BlendOutTime = 0.1f;
		AlphaBoolBlend.BlendOption = EAlphaBlendOption::HermiteCubic;
	}

protected:
	virtual void UpdateBlendSpace(const FAnimationUpdateContext& Context) final;
};