// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.

#include "AnimNodes/AnimNode_MotionWeight.h"


void FAnimNode_MotionWeight::UpdateBlendSpace(const FAnimationUpdateContext& Context)
{
	X = Weight;
}
