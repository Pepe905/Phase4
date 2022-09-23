// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.

#include "AnimGraphNode_LookTarget.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "DetailLayoutBuilder.h"

#define LOCTEXT_NAMESPACE "MoveItEditor_Node"


FText UAnimGraphNode_LookTarget::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return GetControllerDescription();
}

FLinearColor UAnimGraphNode_LookTarget::GetNodeTitleColor() const
{
	return FLinearColor::Black;
}

FText UAnimGraphNode_LookTarget::GetTooltipText() const
{
	return LOCTEXT("AnimGraphNode_LookTarget_Tooltip", "Procedurally looks at a given target or location. Applies a spring for natural & realistic motion.");
}

FString UAnimGraphNode_LookTarget::GetNodeCategory() const
{
	return TEXT("MoveIt!");
}

void UAnimGraphNode_LookTarget::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	const FName PropertyName = (PropertyChangedEvent.Property ? PropertyChangedEvent.Property->GetFName() : NAME_None);

	if (PropertyName == GET_MEMBER_NAME_STRING_CHECKED(FAnimNode_LookTarget, TargetInputType))
	{
		FScopedTransaction Transaction(LOCTEXT("ChangeTargetInputType", "Change Target Input Type"));
		Modify();

		const FAnimNode_LookTarget* LookNode = &Node;

		// Break links to pins going away
		for (int32 PinIndex = 0; PinIndex < Pins.Num(); ++PinIndex)
		{
			UEdGraphPin* Pin = Pins[PinIndex];
			if (Pin->PinName == GET_MEMBER_NAME_STRING_CHECKED(FAnimNode_LookTarget, Location))
			{
				if (LookNode->TargetInputType != EMILookTargetInput::LTI_Location)
				{
					Pin->BreakAllPinLinks();
				}
			}
			else if (Pin->PinName == GET_MEMBER_NAME_STRING_CHECKED(FAnimNode_LookTarget, Target))
			{
				if (LookNode->TargetInputType != EMILookTargetInput::LTI_Actor)
				{
					Pin->BreakAllPinLinks();
				}
			}
		}

		ReconstructNode();

		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(GetBlueprint());
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}

void UAnimGraphNode_LookTarget::CustomizePinData(UEdGraphPin* Pin, FName SourcePropertyName, int32 ArrayIndex) const
{
	Super::CustomizePinData(Pin, SourcePropertyName, ArrayIndex);

	const FAnimNode_LookTarget* LookNode = &Node;

	if (Pin->PinName == GET_MEMBER_NAME_STRING_CHECKED(FAnimNode_LookTarget, Location))
	{
		Pin->bHidden = (LookNode->TargetInputType != EMILookTargetInput::LTI_Location);
	}

	if (Pin->PinName == GET_MEMBER_NAME_STRING_CHECKED(FAnimNode_LookTarget, Target))
	{
		Pin->bHidden = (LookNode->TargetInputType != EMILookTargetInput::LTI_Actor);
	}
}

void UAnimGraphNode_LookTarget::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	Super::CustomizeDetails(DetailBuilder);

	const FAnimNode_LookTarget* LookNode = &Node;

	TSharedRef<IPropertyHandle> NodeHandle = DetailBuilder.GetProperty(FName(TEXT("Node")), GetClass());
	
	if (LookNode->TargetInputType != EMILookTargetInput::LTI_Location)
	{
		DetailBuilder.HideProperty(NodeHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FAnimNode_LookTarget, Location)));
	}

	if (LookNode->TargetInputType != EMILookTargetInput::LTI_Actor)
	{
		DetailBuilder.HideProperty(NodeHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FAnimNode_LookTarget, Target)));
	}
}

FText UAnimGraphNode_LookTarget::GetControllerDescription() const
{
	return LOCTEXT("LookTarget", "Look Target");
}

#undef LOCTEXT_NAMESPACE