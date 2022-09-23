// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.

#include "AnimGraphNode_MotionWeight.h"

#define LOCTEXT_NAMESPACE "MoveItEditor_Node"

FText UAnimGraphNode_MotionWeight::GetControllerDescription() const
{
	return LOCTEXT("MotionWeightNode", "Motion Weight");
}

FText UAnimGraphNode_MotionWeight::GetTooltipText() const
{
	return LOCTEXT("AnimGraphNode_MotionWeight_Tooltip", "Apply weight shift from motion change - such as running fast then stopping suddenly, or starting suddenly.");
}

#undef LOCTEXT_NAMESPACE