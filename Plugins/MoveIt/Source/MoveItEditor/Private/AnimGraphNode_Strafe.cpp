// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.

#include "AnimGraphNode_Strafe.h"

#define LOCTEXT_NAMESPACE "MoveItEditor_Node"


FText UAnimGraphNode_Strafe::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return GetControllerDescription();
}

FLinearColor UAnimGraphNode_Strafe::GetNodeTitleColor() const
{
	return FLinearColor::Black;
}

FText UAnimGraphNode_Strafe::GetTooltipText() const
{
	return LOCTEXT("AnimGraphNode_Strafe_Tooltip", "Procedurally generates strafing animations by reorienting the feet and body.");
}

FString UAnimGraphNode_Strafe::GetNodeCategory() const
{
	return TEXT("MoveIt!");
}

FText UAnimGraphNode_Strafe::GetControllerDescription() const
{
	return LOCTEXT("Strafe", "Strafe");
}

#undef LOCTEXT_NAMESPACE