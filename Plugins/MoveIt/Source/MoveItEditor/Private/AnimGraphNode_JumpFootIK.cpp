// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.

#include "AnimGraphNode_JumpFootIK.h"

#define LOCTEXT_NAMESPACE "MoveItEditor_Node"

FText UAnimGraphNode_JumpFootIK::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return GetControllerDescription();
}

FText UAnimGraphNode_JumpFootIK::GetTooltipText() const
{
	return LOCTEXT("AnimGraphNode_Jump_Tooltip", "Compute best leg to jump off then moves it toward the ground.");
}

FString UAnimGraphNode_JumpFootIK::GetNodeCategory() const
{
	return TEXT("MoveIt!");
}

FText UAnimGraphNode_JumpFootIK::GetControllerDescription() const
{
	return LOCTEXT("Jump", "Foot IK (Jumping)");
}

#undef LOCTEXT_NAMESPACE