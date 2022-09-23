// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.

#include "AnimGraphNode_LocalSkeletalControlBase.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Animation/AnimationSettings.h"
#include "Kismet2/CompilerResultsLog.h"
#include "DetailLayoutBuilder.h"

#define LOCTEXT_NAMESPACE "MoveItEditor_Node"

FLinearColor UAnimGraphNode_LocalSkeletalControlBase::GetNodeTitleColor() const
{
	return FLinearColor::Black;
}

FText UAnimGraphNode_LocalSkeletalControlBase::GetTooltipText() const
{
	return GetControllerDescription();
}

void UAnimGraphNode_LocalSkeletalControlBase::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	const FName PropertyName = (PropertyChangedEvent.Property ? PropertyChangedEvent.Property->GetFName() : NAME_None);

	// Reconstruct node to show updates to PinFriendlyNames.
	if ((PropertyName == GET_MEMBER_NAME_STRING_CHECKED(FAnimNode_LocalSkeletalControlBase, AlphaScaleBias))
		|| (PropertyName == GET_MEMBER_NAME_STRING_CHECKED(FInputScaleBiasClamp, bMapRange))
		|| (PropertyName == GET_MEMBER_NAME_STRING_CHECKED(FInputRange, Min))
		|| (PropertyName == GET_MEMBER_NAME_STRING_CHECKED(FInputRange, Max))
		|| (PropertyName == GET_MEMBER_NAME_STRING_CHECKED(FInputScaleBiasClamp, Scale))
		|| (PropertyName == GET_MEMBER_NAME_STRING_CHECKED(FInputScaleBiasClamp, Bias))
		|| (PropertyName == GET_MEMBER_NAME_STRING_CHECKED(FInputScaleBiasClamp, bClampResult))
		|| (PropertyName == GET_MEMBER_NAME_STRING_CHECKED(FInputScaleBiasClamp, ClampMin))
		|| (PropertyName == GET_MEMBER_NAME_STRING_CHECKED(FInputScaleBiasClamp, ClampMax))
		|| (PropertyName == GET_MEMBER_NAME_STRING_CHECKED(FInputScaleBiasClamp, bInterpResult))
		|| (PropertyName == GET_MEMBER_NAME_STRING_CHECKED(FInputScaleBiasClamp, InterpSpeedIncreasing))
		|| (PropertyName == GET_MEMBER_NAME_STRING_CHECKED(FInputScaleBiasClamp, InterpSpeedDecreasing)))
	{
		ReconstructNode();
	}

	if (PropertyName == GET_MEMBER_NAME_STRING_CHECKED(FAnimNode_LocalSkeletalControlBase, AlphaInputType))
	{
		FScopedTransaction Transaction(LOCTEXT("ChangeAlphaInputType", "Change Alpha Input Type"));
		Modify();

		const FAnimNode_LocalSkeletalControlBase* SkelControlNode = GetNode();

		// Break links to pins going away
		for (int32 PinIndex = 0; PinIndex < Pins.Num(); ++PinIndex)
		{
			UEdGraphPin* Pin = Pins[PinIndex];
			if (Pin->PinName == GET_MEMBER_NAME_STRING_CHECKED(FAnimNode_LocalSkeletalControlBase, Alpha))
			{
				if (GetNode()->AlphaInputType != EAnimAlphaInputType::Float)
				{
					Pin->BreakAllPinLinks();
				}
			}
			else if (Pin->PinName == GET_MEMBER_NAME_STRING_CHECKED(FAnimNode_LocalSkeletalControlBase, bAlphaBoolEnabled))
			{
				if (GetNode()->AlphaInputType != EAnimAlphaInputType::Bool)
				{
					Pin->BreakAllPinLinks();
				}
			}
			else if (Pin->PinName == GET_MEMBER_NAME_STRING_CHECKED(FAnimNode_LocalSkeletalControlBase, AlphaCurveName))
			{
				if (GetNode()->AlphaInputType != EAnimAlphaInputType::Curve)
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

FString UAnimGraphNode_LocalSkeletalControlBase::GetNodeCategory() const
{
	return TEXT("Skeletal Control Nodes");
}

void UAnimGraphNode_LocalSkeletalControlBase::ValidateAnimNodePostCompile(FCompilerResultsLog& MessageLog, UAnimBlueprintGeneratedClass* CompiledClass, int32 CompiledNodeIndex)
{
	if (UAnimationSettings::Get()->bEnablePerformanceLog)
	{
		const FAnimNode_LocalSkeletalControlBase* Node = GetNode();
		if (Node && Node->LODThreshold < 0)
		{
			MessageLog.Warning(TEXT("@@ contains no LOD Threshold."), this);
		}
	}
}

void UAnimGraphNode_LocalSkeletalControlBase::CustomizePinData(UEdGraphPin* Pin, FName SourcePropertyName, int32 ArrayIndex) const
{
	Super::CustomizePinData(Pin, SourcePropertyName, ArrayIndex);

	if (Pin->PinName == GET_MEMBER_NAME_STRING_CHECKED(FAnimNode_LocalSkeletalControlBase, Alpha))
	{
		Pin->bHidden = (GetNode()->AlphaInputType != EAnimAlphaInputType::Float);

		if (!Pin->bHidden)
		{
			Pin->PinFriendlyName = GetNode()->AlphaScaleBias.GetFriendlyName(GetNode()->AlphaScaleBiasClamp.GetFriendlyName(Pin->PinFriendlyName));
		}
	}

	if (Pin->PinName == GET_MEMBER_NAME_STRING_CHECKED(FAnimNode_LocalSkeletalControlBase, bAlphaBoolEnabled))
	{
		Pin->bHidden = (GetNode()->AlphaInputType != EAnimAlphaInputType::Bool);
	}

	if (Pin->PinName == GET_MEMBER_NAME_STRING_CHECKED(FAnimNode_LocalSkeletalControlBase, AlphaCurveName))
	{
		Pin->bHidden = (GetNode()->AlphaInputType != EAnimAlphaInputType::Curve);

		if (!Pin->bHidden)
		{
			Pin->PinFriendlyName = GetNode()->AlphaScaleBiasClamp.GetFriendlyName(Pin->PinFriendlyName);
		}
	}

	if (Pin->PinName == GET_MEMBER_NAME_STRING_CHECKED(FAnimNode_LocalSkeletalControlBase, SourcePose))
	{
		//Pin->bHidden = true;
		//Pin->PinFriendlyName = FText::FromString("Bad Link");
	}
}

void UAnimGraphNode_LocalSkeletalControlBase::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	Super::CustomizeDetails(DetailBuilder);

	const FAnimNode_LocalSkeletalControlBase* SkelControlNode = GetNode();
	TSharedRef<IPropertyHandle> NodeHandle = DetailBuilder.GetProperty(FName(TEXT("Node")), GetClass());

	if (SkelControlNode->AlphaInputType != EAnimAlphaInputType::Bool)
	{
		DetailBuilder.HideProperty(NodeHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FAnimNode_LocalSkeletalControlBase, bAlphaBoolEnabled)));
		DetailBuilder.HideProperty(NodeHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FAnimNode_LocalSkeletalControlBase, AlphaBoolBlend)));
	}

	if (SkelControlNode->AlphaInputType != EAnimAlphaInputType::Float)
	{
		DetailBuilder.HideProperty(NodeHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FAnimNode_LocalSkeletalControlBase, Alpha)));
		DetailBuilder.HideProperty(NodeHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FAnimNode_LocalSkeletalControlBase, AlphaScaleBias)));
	}

	if (SkelControlNode->AlphaInputType != EAnimAlphaInputType::Curve)
	{
		DetailBuilder.HideProperty(NodeHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FAnimNode_LocalSkeletalControlBase, AlphaCurveName)));
	}

	if ((SkelControlNode->AlphaInputType != EAnimAlphaInputType::Float)
		&& (SkelControlNode->AlphaInputType != EAnimAlphaInputType::Curve))
	{
		DetailBuilder.HideProperty(NodeHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FAnimNode_LocalSkeletalControlBase, AlphaScaleBiasClamp)));
	}
}

bool UAnimGraphNode_LocalSkeletalControlBase::IsPinShown(const FName PinName) const
{
	for (const FOptionalPinFromProperty& Pin : ShowPinForProperties)
	{
		if (Pin.PropertyName == PinName)
		{
			return Pin.bShowPin;
		}
	}
	return false;
}

void UAnimGraphNode_LocalSkeletalControlBase::SetDefaultValue(const FName UpdateDefaultValueName, const FVector& Value)
{
	for (UEdGraphPin* Pin : Pins)
	{
		if (Pin->PinName == UpdateDefaultValueName)
		{
			if (GetSchema()->IsCurrentPinDefaultValid(Pin).IsEmpty())
			{
				FString Str = FString::Printf(TEXT("%.3f,%.3f,%.3f"), Value.X, Value.Y, Value.Z);
				if (Pin->DefaultValue != Str)
				{
					PreEditChange(nullptr);
					GetSchema()->TrySetDefaultValue(*Pin, Str);
					PostEditChange();
					break;
				}
			}
		}
	}
}

void UAnimGraphNode_LocalSkeletalControlBase::GetDefaultValue(const FName UpdateDefaultValueName, FVector& OutVec)
{
	for (UEdGraphPin* Pin : Pins)
	{
		if (Pin->PinName == UpdateDefaultValueName)
		{
			if (GetSchema()->IsCurrentPinDefaultValid(Pin).IsEmpty())
			{
				FString DefaultString = Pin->GetDefaultAsString();

				// Existing nodes (from older versions) might have an empty default value string; in that case we just fall through and return the zero vector below (which is the default value in that case).
				if (!DefaultString.IsEmpty())
				{
					TArray<FString> ResultString;

					//Parse string to split its contents separated by ','
					DefaultString.TrimStartAndEndInline();
					DefaultString.ParseIntoArray(ResultString, TEXT(","), true);

					check(ResultString.Num() == 3);

					OutVec.Set(
						FCString::Atof(*ResultString[0]),
						FCString::Atof(*ResultString[1]),
						FCString::Atof(*ResultString[2])
					);
					return;
				}
			}
		}
	}
	OutVec = FVector::ZeroVector;
}

FText UAnimGraphNode_LocalSkeletalControlBase::GetControllerDescription() const
{
	return LOCTEXT("ImplementMe", "Implement me");
}

#undef LOCTEXT_NAMESPACE