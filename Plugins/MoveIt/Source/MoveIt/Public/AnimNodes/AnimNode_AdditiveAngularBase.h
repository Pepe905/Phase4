// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.

#pragma once

#include "AnimNodes/AnimNode_AdditiveBlendSpace.h"
#include "MITypes.h"
#include "AnimNode_AdditiveAngularBase.generated.h"


/**
 *	Computes angular velocity for usage with a blendspace, which then does the equivalent of "Apply Additive"
 */
USTRUCT(BlueprintInternalUseOnly)
struct MOVEIT_API FAnimNode_AdditiveAngularBase : public FAnimNode_AdditiveBlendSpace
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = AngularVelocity, meta = (PinShownByDefault))
	FRotator AngularVelocity;

	// SCALE: COMMON SETTING [NOT IMPLEMENTED BY DEFAULT]

	/** Scale angular velocity by this amount */
	UPROPERTY(EditAnywhere, Category = AngularVelocity, meta = (PinHiddenByDefault))
	float Scale;

public:
	FAnimNode_AdditiveAngularBase()
		: AngularVelocity(FRotator::ZeroRotator)
		, Scale(1.f)
	{}
};