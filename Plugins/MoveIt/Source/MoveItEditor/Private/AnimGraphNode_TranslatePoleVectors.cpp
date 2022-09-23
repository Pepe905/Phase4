// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.

#include "AnimGraphNode_TranslatePoleVectors.h"

#define LOCTEXT_NAMESPACE "MoveItEditor_Node"

FText UAnimGraphNode_TranslatePoleVectors::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return GetControllerDescription();
}

FText UAnimGraphNode_TranslatePoleVectors::GetTooltipText() const
{
	return LOCTEXT("AnimGraphNode_TranslatePoleVectors_Tooltip", "Compute and apply ideal pole vector location for arms.");
}

FString UAnimGraphNode_TranslatePoleVectors::GetNodeCategory() const
{
	return TEXT("MoveIt!");
}

FText UAnimGraphNode_TranslatePoleVectors::GetControllerDescription() const
{
	return LOCTEXT("TranslatePoleVectors", "Translate Pole Vectors");
}

#undef LOCTEXT_NAMESPACE