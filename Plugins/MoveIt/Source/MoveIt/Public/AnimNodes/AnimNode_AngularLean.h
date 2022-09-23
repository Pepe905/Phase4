// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.

#pragma once

#include "AnimNodes/AnimNode_AdditiveAngularBase.h"
#include "AnimNodes/AnimNode_BlendSpacePlayer.h"
#include "AnimNode_AngularLean.generated.h"


/**
 *	Apply angular velocity to an additive leaning blendspace
 */
USTRUCT(BlueprintInternalUseOnly)
struct MOVEIT_API FAnimNode_AngularLean : public FAnimNode_AdditiveAngularBase
{
	GENERATED_BODY()

protected:
	FVector Velocity;
	float Speed;
	float NormalizedSpeed;

public:
	FAnimNode_AngularLean()
		: Velocity(FVector::ZeroVector)
		, Speed(0.f)
		, NormalizedSpeed(0.f)
	{
		Scale = 0.001f;
	}

public:
	virtual void PreUpdate(const UAnimInstance* InAnimInstance) final;

protected:
	virtual void UpdateBlendSpace(const FAnimationUpdateContext& Context) final;
};