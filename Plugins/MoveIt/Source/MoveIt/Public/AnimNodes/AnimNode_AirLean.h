// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.

#pragma once

#include "AnimNodes/AnimNode_AdditiveBlendSpace.h"
#include "AnimNode_AirLean.generated.h"

class ACharacter;


/**
 *	Additive leaning while in air to compensate for velocity
 */
USTRUCT(BlueprintInternalUseOnly)
struct MOVEIT_API FAnimNode_AirLean : public FAnimNode_AdditiveBlendSpace
{
	GENERATED_BODY()

protected:
	UPROPERTY()
	ACharacter* Character;

public:
	FAnimNode_AirLean()
		: Character(nullptr)
	{}

protected:
	virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) final;
	virtual void UpdateBlendSpace(const FAnimationUpdateContext& Context) final;
};