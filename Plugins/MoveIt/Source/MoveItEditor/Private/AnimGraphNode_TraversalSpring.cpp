// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.

#include "AnimGraphNode_TraversalSpring.h"

#define LOCTEXT_NAMESPACE "MoveItEditor_Node"

FText UAnimGraphNode_TraversalSpring::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return GetControllerDescription();
}

FText UAnimGraphNode_TraversalSpring::GetTooltipText() const
{
	return LOCTEXT("AnimGraphNode_TraversalSpring_Tooltip", "Applies a spring based on movement while on an incline to push the pelvis up (walking uphill) or down (walking downhill).");
}

FString UAnimGraphNode_TraversalSpring::GetNodeCategory() const
{
	return TEXT("MoveIt!");
}

FText UAnimGraphNode_TraversalSpring::GetControllerDescription() const
{
	return LOCTEXT("TraversalSpring", "Traversal (Z) Spring");
}

#undef LOCTEXT_NAMESPACE