// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.

#include "AnimGraphNode_AirLean.h"

#define LOCTEXT_NAMESPACE "MoveItEditor_Node"

FText UAnimGraphNode_AirLean::GetControllerDescription() const
{
	return LOCTEXT("AirLeanNode", "Air Lean");
}

FText UAnimGraphNode_AirLean::GetTooltipText() const
{
	return LOCTEXT("AnimGraphNode_AirLean_Tooltip", "Additive leaning while in air to compensate for velocity.");
}

#undef LOCTEXT_NAMESPACE