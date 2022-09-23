// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.

#include "AnimGraphNode_Duck.h"

#define LOCTEXT_NAMESPACE "MoveItEditor_Node"

FText UAnimGraphNode_Duck::GetControllerDescription() const
{
	return LOCTEXT("DuckNode", "Duck");
}

FText UAnimGraphNode_Duck::GetTooltipText() const
{
	return LOCTEXT("AnimGraphNode_Duck_Tooltip", "Ducks under obstacles.");
}

#undef LOCTEXT_NAMESPACE