// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.

#include "AnimGraphNode_CopyIKBones.h"

#define LOCTEXT_NAMESPACE "MoveItEditor_Node"

FText UAnimGraphNode_CopyIKBones::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return GetControllerDescription();
}

FText UAnimGraphNode_CopyIKBones::GetTooltipText() const
{
	return LOCTEXT("AnimGraphNode_CopyIKBones_Tooltip", "Copies the IK bones to the matching bones. This helps a lot with animations that didn't animate the IK bones to match.");
}

FString UAnimGraphNode_CopyIKBones::GetNodeCategory() const
{
	return TEXT("MoveIt!");
}

FText UAnimGraphNode_CopyIKBones::GetControllerDescription() const
{
	return LOCTEXT("CopyIKBones", "Copy IK Bones");
}

#undef LOCTEXT_NAMESPACE