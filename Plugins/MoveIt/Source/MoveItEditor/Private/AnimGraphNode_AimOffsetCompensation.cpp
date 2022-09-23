// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.

#include "AnimGraphNode_AimOffsetCompensation.h"

#define LOCTEXT_NAMESPACE "MoveItEditor_Node"

FText UAnimGraphNode_AimOffsetCompensation::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return GetControllerDescription();
}

FText UAnimGraphNode_AimOffsetCompensation::GetTooltipText() const
{
	return LOCTEXT("AnimGraphNode_AimOffsetCompensation_Tooltip", "Rotates bones to match the pose negated by Turn In Place so that Aim Offset lines up.");
}

FString UAnimGraphNode_AimOffsetCompensation::GetNodeCategory() const
{
	return TEXT("MoveIt!");
}

FText UAnimGraphNode_AimOffsetCompensation::GetControllerDescription() const
{
	return LOCTEXT("AimOffsetCompensation", "Aim Offset Compensation");
}

#undef LOCTEXT_NAMESPACE