// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.

#include "AnimGraphNode_LandingPose.h"

#define LOCTEXT_NAMESPACE "MoveItEditor_Node"

FText UAnimGraphNode_LandingPose::GetControllerDescription() const
{
	return LOCTEXT("LandingPoseNode", "LandingPose");
}

FText UAnimGraphNode_LandingPose::GetTooltipText() const
{
	return LOCTEXT("AnimGraphNode_LandingPose_Tooltip", "Predicts where the character will land and adapts pose based on distance to ground and movement direction.");
}

#undef LOCTEXT_NAMESPACE