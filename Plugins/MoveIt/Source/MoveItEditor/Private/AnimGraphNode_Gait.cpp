// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.

#include "AnimGraphNode_Gait.h"

#define LOCTEXT_NAMESPACE "MoveItEditor_Node"

FText UAnimGraphNode_Gait::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return GetControllerDescription();
}

FText UAnimGraphNode_Gait::GetTooltipText() const
{
	return LOCTEXT("AnimGraphNode_Gait_Tooltip", "Changes the distance between the feet while moving, providing partial start and stop animations and helps to prevent legs clipping walls. Applies a spring to the pelvis to modify the character's motion weight at shorter gaits.");
}

FString UAnimGraphNode_Gait::GetNodeCategory() const
{
	return TEXT("MoveIt!");
}

FText UAnimGraphNode_Gait::GetControllerDescription() const
{
	return LOCTEXT("Gait", "Gait");
}

#undef LOCTEXT_NAMESPACE