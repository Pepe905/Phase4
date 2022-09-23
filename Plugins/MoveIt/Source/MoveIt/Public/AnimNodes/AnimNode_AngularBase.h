// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.

#pragma once

#include "AnimNodes/AnimNode_LocalSkeletalControlBase.h"
#include "AnimNodes/AnimNodes_Shared.h"
#include "MITypes.h"
#include "AnimNode_AngularBase.generated.h"

/**
 * Base for nodes applying angular velocity to calculations and performing the equivalent 
 * of "Local to Component" and "Component to Local" (very expensive) using a LUT (very cheap) instead.
 */
USTRUCT(BlueprintInternalUseOnly)
struct MOVEIT_API FAnimNode_AngularBase : public FAnimNode_LocalSkeletalControlBase
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
	FAnimNode_AngularBase()
		: AngularVelocity(FRotator::ZeroRotator)
		, Scale(1.f)
	{}
};