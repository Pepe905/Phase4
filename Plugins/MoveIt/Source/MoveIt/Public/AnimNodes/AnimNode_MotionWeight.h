// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.

#pragma once

#include "AnimNodes/AnimNode_AdditiveBlendSpace.h"
#include "AnimNode_MotionWeight.generated.h"


/**
 *	Apply weight shift from motion change - such as running fast then stopping suddenly, or starting suddenly
 *	This is just a shell that applies additive blendspace, the real magic happens elsewhere
 */
USTRUCT(BlueprintInternalUseOnly)
struct MOVEIT_API FAnimNode_MotionWeight : public FAnimNode_AdditiveBlendSpace
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Direction, meta = (PinShownByDefault))
	float Weight;

public:
	FAnimNode_MotionWeight()
		: Weight(0.f)
	{}

protected:
	virtual void UpdateBlendSpace(const FAnimationUpdateContext& Context) final;
};