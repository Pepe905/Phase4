// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.


#include "AnimGraphNode_SpringBase.h"


#define LOCTEXT_NAMESPACE "A3Nodes"


FText UAnimGraphNode_SpringBase::GetControllerDescription() const
{
	return LOCTEXT("LimitedSpringController", "Spring Controller");
}

FText UAnimGraphNode_SpringBase::GetTooltipText() const
{
	return LOCTEXT("AnimGraphNode_SpringBase_Tooltip", "The Spring Controller applies a spring solver that can be used to limit how far a bone can stretch from its reference pose position and apply a force in the opposite direction. This modififed version can limit individual axis.");
}

FString UAnimGraphNode_SpringBase::GetNodeCategory() const
{
	return TEXT("MoveIt!");
}

FText UAnimGraphNode_SpringBase::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if ((TitleType == ENodeTitleType::ListView || TitleType == ENodeTitleType::MenuTitle) && (Node.SpringBone.BoneName == NAME_None))
	{
		return GetControllerDescription();
	}
	// @TODO: the bone can be altered in the property editor, so we have to 
	//        choose to mark this dirty when that happens for this to properly work
	else //if(!CachedNodeTitles.IsTitleCached(TitleType, this))
	{
		FFormatNamedArguments Args;
		Args.Add(TEXT("ControllerDescription"), GetControllerDescription());
		Args.Add(TEXT("BoneName"), FText::FromName(Node.SpringBone.BoneName));

		// FText::Format() is slow, so we cache this to save on performance
		if (TitleType == ENodeTitleType::ListView || TitleType == ENodeTitleType::MenuTitle)
		{
			CachedNodeTitles.SetCachedTitle(TitleType, FText::Format(LOCTEXT("AnimGraphNode_SpringBone_ListTitle", "{ControllerDescription} - Bone: {BoneName}"), Args), this);
		}
		else
		{
			CachedNodeTitles.SetCachedTitle(TitleType, FText::Format(LOCTEXT("AnimGraphNode_SpringBone_Title", "{ControllerDescription}\nBone: {BoneName}"), Args), this);
		}
	}

	return CachedNodeTitles[TitleType];
}

#undef LOCTEXT_NAMESPACE
