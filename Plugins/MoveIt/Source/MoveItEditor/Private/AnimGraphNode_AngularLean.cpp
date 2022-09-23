// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.

#include "AnimGraphNode_AngularLean.h"


#define LOCTEXT_NAMESPACE "MoveItEditor_Node"

FText UAnimGraphNode_Lean::GetControllerDescription() const
{
	return LOCTEXT("LeanNode", "Angular Lean");
}

FText UAnimGraphNode_Lean::GetTooltipText() const
{
	return LOCTEXT("AnimGraphNode_Lean_Tooltip", "Apply angular velocity to an additive leaning blendspace.");
}

#undef LOCTEXT_NAMESPACE