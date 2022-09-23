// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.

#include "AnimGraphNode_LandFootIK.h"
#include "PropertyHandle.h"
#include "DetailLayoutBuilder.h"

#define LOCTEXT_NAMESPACE "MoveItEditor_Node"

FText UAnimGraphNode_LandFootIK::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return GetControllerDescription();
}

FText UAnimGraphNode_LandFootIK::GetTooltipText() const
{
	return LOCTEXT("AnimGraphNode_Land_Tooltip", "Plants the feet at the predicted landing location.");
}

FString UAnimGraphNode_LandFootIK::GetNodeCategory() const
{
	return TEXT("MoveIt!");
}

void UAnimGraphNode_LandFootIK::CustomizePinData(UEdGraphPin* Pin, FName SourcePropertyName, int32 ArrayIndex) const
{
	Super::CustomizePinData(Pin, SourcePropertyName, ArrayIndex);

	if (Pin->PinName == GET_MEMBER_NAME_STRING_CHECKED(FAnimNode_LandFootIK, bAlphaBoolEnabled))
	{
		Pin->bHidden = true;
	}
}

void UAnimGraphNode_LandFootIK::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	Super::CustomizeDetails(DetailBuilder);

	TSharedRef<IPropertyHandle> NodeHandle = DetailBuilder.GetProperty(FName(TEXT("Node")), GetClass());

	DetailBuilder.HideProperty(NodeHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FAnimNode_LandFootIK, AlphaInputType)));
	DetailBuilder.HideProperty(NodeHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FAnimNode_LandFootIK, bAlphaBoolEnabled)));
}

FText UAnimGraphNode_LandFootIK::GetControllerDescription() const
{
	return LOCTEXT("Land", "Foot IK (Landing)");
}

#undef LOCTEXT_NAMESPACE