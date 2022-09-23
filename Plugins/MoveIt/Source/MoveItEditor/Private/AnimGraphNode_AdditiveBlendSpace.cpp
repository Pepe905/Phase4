// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.

#include "AnimGraphNode_AdditiveBlendSpace.h"
#include "Kismet2/CompilerResultsLog.h"
#include "GraphEditorActions.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "AssetRegistryModule.h"
#include "BlueprintNodeSpawner.h"
#include "Animation/BlendSpace.h"
#include "Animation/BlendSpace1D.h"
#include "Animation/AimOffsetBlendSpace.h"
#include "Animation/AimOffsetBlendSpace1D.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Animation/AnimationSettings.h"
#include "DetailLayoutBuilder.h"

#define LOCTEXT_NAMESPACE "MoveItEditor_Node"

FText UAnimGraphNode_AdditiveBlendSpace::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if ((TitleType == ENodeTitleType::ListView || TitleType == ENodeTitleType::MenuTitle)  || !GetNode())
	{
		return GetControllerDescription();
	}
	// @TODO: the bone can be altered in the property editor, so we have to 
	//        choose to mark this dirty when that happens for this to properly work
	else //if(!CachedNodeTitles.IsTitleCached(TitleType, this))
	{
		FFormatNamedArguments Args;
		Args.Add(TEXT("ControllerDescription"), GetControllerDescription());
		Args.Add(TEXT("Type"), FText::FromName(GetNode()->GetAdditiveName()));

		// FText::Format() is slow, so we cache this to save on performance
		if (TitleType == ENodeTitleType::ListView || TitleType == ENodeTitleType::MenuTitle)
		{
			CachedNodeTitles.SetCachedTitle(TitleType, FText::Format(LOCTEXT("AnimGraphNode_AdditiveBlendSpace_ListTitle", "{ControllerDescription} - Type: {Type}"), Args), this);
		}
		else
		{
			CachedNodeTitles.SetCachedTitle(TitleType, FText::Format(LOCTEXT("AnimGraphNode_AdditiveBlendSpace_ListTitle", "{ControllerDescription}\nType: {Type}"), Args), this);
		}
	}

	return CachedNodeTitles[TitleType];
}

FLinearColor UAnimGraphNode_AdditiveBlendSpace::GetNodeTitleColor() const
{
	return FLinearColor::Black;
}

FString UAnimGraphNode_AdditiveBlendSpace::GetNodeCategory() const
{
	return TEXT("MoveIt!");
}

void UAnimGraphNode_AdditiveBlendSpace::PinConnectionListChanged(UEdGraphPin* Pin)
{
	Super::PinConnectionListChanged(Pin);

	if (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Object)
	{
		// recache visualization now an asset pin's connection is changed
		if (const UEdGraphSchema * Schema = GetSchema())
		{
			Schema->ForceVisualizationCacheClear();
		}
	}
}

void UAnimGraphNode_AdditiveBlendSpace::PinDefaultValueChanged(UEdGraphPin* Pin)
{
	Super::PinDefaultValueChanged(Pin);

	if (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Object)
	{
		// recache visualization now an asset pin's default value has changed
		if (const UEdGraphSchema * Schema = GetSchema())
		{
			Schema->ForceVisualizationCacheClear();
		}
	}
}

void UAnimGraphNode_AdditiveBlendSpace::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	const FName PropertyName = (PropertyChangedEvent.Property ? PropertyChangedEvent.Property->GetFName() : NAME_None);

	// Reconstruct node to show updates to PinFriendlyNames.
	if ((PropertyName == GET_MEMBER_NAME_STRING_CHECKED(FAnimNode_AdditiveBlendSpace, AlphaScaleBias))
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

	if (PropertyName == GET_MEMBER_NAME_STRING_CHECKED(FAnimNode_AdditiveBlendSpace, AlphaInputType))
	{
		FScopedTransaction Transaction(LOCTEXT("ChangeAlphaInputType", "Change Alpha Input Type"));
		Modify();

		// Break links to pins going away
		for (int32 PinIndex = 0; PinIndex < Pins.Num(); ++PinIndex)
		{
			UEdGraphPin* Pin = Pins[PinIndex];
			if (Pin->PinName == GET_MEMBER_NAME_STRING_CHECKED(FAnimNode_AdditiveBlendSpace, Alpha))
			{
				if (GetNode()->AlphaInputType != EAnimAlphaInputType::Float)
				{
					Pin->BreakAllPinLinks();
				}
			}
			else if (Pin->PinName == GET_MEMBER_NAME_STRING_CHECKED(FAnimNode_AdditiveBlendSpace, bAlphaBoolEnabled))
			{
				if (GetNode()->AlphaInputType != EAnimAlphaInputType::Bool)
				{
					Pin->BreakAllPinLinks();
				}
			}
			else if (Pin->PinName == GET_MEMBER_NAME_STRING_CHECKED(FAnimNode_AdditiveBlendSpace, AlphaCurveName))
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

FText UAnimGraphNode_AdditiveBlendSpace::GetControllerDescription() const
{
	return LOCTEXT("AnimGraphNodeAdditiveBlendSpaceTitle", "Additive BlendSpace Player");
}

void UAnimGraphNode_AdditiveBlendSpace::ValidateAnimNodeDuringCompilation(class USkeleton* ForSkeleton, class FCompilerResultsLog& MessageLog)
{
	Super::ValidateAnimNodeDuringCompilation(ForSkeleton, MessageLog);

	// Blendspace
	UBlendSpace* BlendSpaceToCheck = GetNode()->GetBlendSpace();
	UEdGraphPin* BlendSpacePin = FindPin(GET_MEMBER_NAME_STRING_CHECKED(FAnimNode_MIBlendSpacePlayer, BlendSpace));
	if (BlendSpacePin != nullptr && BlendSpaceToCheck == nullptr)
	{
		BlendSpaceToCheck = Cast<UBlendSpace>(BlendSpacePin->DefaultObject);
	}

	if (BlendSpaceToCheck == nullptr)
	{
		// we may have a connected node
		if (BlendSpacePin == nullptr || BlendSpacePin->LinkedTo.Num() == 0)
		{
			MessageLog.Error(TEXT("@@ references an unknown blend space"), this);
		}
	}
	else
	{
		USkeleton* BlendSpaceSkeleton = BlendSpaceToCheck->GetSkeleton();
		if (BlendSpaceSkeleton && // if blend space doesn't have skeleton, it might be due to blend space not loaded yet, @todo: wait with anim blueprint compilation until all assets are loaded?
			!BlendSpaceSkeleton->IsCompatible(ForSkeleton))
		{
			MessageLog.Error(TEXT("@@ references blendspace that uses different skeleton @@"), this, BlendSpaceSkeleton);
		}
	}

	// Apply additive
	if (UAnimationSettings::Get()->bEnablePerformanceLog)
	{
		if (GetNode()->LODThreshold < 0)
		{
			MessageLog.Warning(TEXT("@@ contains no LOD Threshold."), this);
		}
	}
}

void UAnimGraphNode_AdditiveBlendSpace::BakeDataDuringCompilation(class FCompilerResultsLog& MessageLog)
{
	UAnimBlueprint* AnimBlueprint = GetAnimBlueprint();
	AnimBlueprint->FindOrAddGroup(SyncGroup.GroupName);
	GetNode()->SetGroupName(SyncGroup.GroupName);
	GetNode()->SetGroupRole(SyncGroup.GroupRole);
	//GetNode()->GroupScope = SyncGroup.GroupScope;
}

bool UAnimGraphNode_AdditiveBlendSpace::DoesSupportTimeForTransitionGetter() const
{
	return true;
}

UAnimationAsset* UAnimGraphNode_AdditiveBlendSpace::GetAnimationAsset() const
{
	UBlendSpace* BlendSpace = GetNode()->GetBlendSpace();
	UEdGraphPin* BlendSpacePin = FindPin(GET_MEMBER_NAME_STRING_CHECKED(FAnimNode_MIBlendSpacePlayer, BlendSpace));
	if (BlendSpacePin != nullptr && BlendSpace == nullptr)
	{
		BlendSpace = Cast<UBlendSpace>(BlendSpacePin->DefaultObject);
	}

	return BlendSpace;
}

const TCHAR* UAnimGraphNode_AdditiveBlendSpace::GetTimePropertyName() const
{
	return TEXT("InternalTimeAccumulator");
}

UScriptStruct* UAnimGraphNode_AdditiveBlendSpace::GetTimePropertyStruct() const
{
	return FAnimNode_MIBlendSpacePlayer::StaticStruct();
}

void UAnimGraphNode_AdditiveBlendSpace::GetAllAnimationSequencesReferred(TArray<UAnimationAsset*>& AnimationAssets) const
{
	if (GetNode()->GetBlendSpace())
	{
		HandleAnimReferenceCollection(GetNode()->GetBlendSpace(), AnimationAssets);
	}
}

void UAnimGraphNode_AdditiveBlendSpace::ReplaceReferredAnimations(const TMap<UAnimationAsset*, UAnimationAsset*>& AnimAssetReplacementMap)
{
	UBlendSpace* BlendSpace = GetNode()->GetBlendSpace();
	HandleAnimReferenceReplacement(BlendSpace, AnimAssetReplacementMap);
	GetNode()->SetBlendSpace(BlendSpace);
}

EAnimAssetHandlerType UAnimGraphNode_AdditiveBlendSpace::SupportsAssetClass(const UClass* AssetClass) const
{
	if (AssetClass->IsChildOf(UBlendSpace::StaticClass()) && !IsAimOffsetBlendSpace(AssetClass))
	{
		return EAnimAssetHandlerType::PrimaryHandler;
	}
	else
	{
		return EAnimAssetHandlerType::NotSupported;
	}
}

void UAnimGraphNode_AdditiveBlendSpace::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	// Apply additive
	Super::CustomizeDetails(DetailBuilder);

	TSharedRef<IPropertyHandle> NodeHandle = DetailBuilder.GetProperty(FName(TEXT("Node")), GetClass());

	if (GetNode()->AlphaInputType != EAnimAlphaInputType::Bool)
	{
		DetailBuilder.HideProperty(NodeHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FAnimNode_AdditiveBlendSpace, bAlphaBoolEnabled)));
		DetailBuilder.HideProperty(NodeHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FAnimNode_AdditiveBlendSpace, AlphaBoolBlend)));
	}

	if (GetNode()->AlphaInputType != EAnimAlphaInputType::Float)
	{
		DetailBuilder.HideProperty(NodeHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FAnimNode_AdditiveBlendSpace, Alpha)));
		DetailBuilder.HideProperty(NodeHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FAnimNode_AdditiveBlendSpace, AlphaScaleBias)));
	}

	if (GetNode()->AlphaInputType != EAnimAlphaInputType::Curve)
	{
		DetailBuilder.HideProperty(NodeHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FAnimNode_AdditiveBlendSpace, AlphaCurveName)));
	}

	if ((GetNode()->AlphaInputType != EAnimAlphaInputType::Float)
		&& (GetNode()->AlphaInputType != EAnimAlphaInputType::Curve))
	{
		DetailBuilder.HideProperty(NodeHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FAnimNode_AdditiveBlendSpace, AlphaScaleBiasClamp)));
	}

	// Blendspace
	DetailBuilder.HideProperty(NodeHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FAnimNode_AdditiveBlendSpace, X)));
	DetailBuilder.HideProperty(NodeHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FAnimNode_AdditiveBlendSpace, Y)));
}

void UAnimGraphNode_AdditiveBlendSpace::CustomizePinData(UEdGraphPin* Pin, FName SourcePropertyName, int32 ArrayIndex) const
{
	// Blendspace
	if (SourcePropertyName == TEXT("X"))
	{
		Pin->bHidden = true;
	}
	else if (SourcePropertyName == TEXT("Y"))
	{
		Pin->bHidden = true;
	}
	else if (SourcePropertyName == TEXT("Z"))
	{
		Pin->bHidden = true;
	}

	// Apply additive
	if (Pin->PinName == GET_MEMBER_NAME_STRING_CHECKED(FAnimNode_AdditiveBlendSpace, Alpha))
	{
		Pin->bHidden = (GetNode()->AlphaInputType != EAnimAlphaInputType::Float);

		if (!Pin->bHidden)
		{
			Pin->PinFriendlyName = GetNode()->AlphaScaleBias.GetFriendlyName(GetNode()->AlphaScaleBiasClamp.GetFriendlyName(Pin->PinFriendlyName));
		}
	}

	if (Pin->PinName == GET_MEMBER_NAME_STRING_CHECKED(FAnimNode_AdditiveBlendSpace, bAlphaBoolEnabled))
	{
		Pin->bHidden = (GetNode()->AlphaInputType != EAnimAlphaInputType::Bool);
	}

	if (Pin->PinName == GET_MEMBER_NAME_STRING_CHECKED(FAnimNode_AdditiveBlendSpace, AlphaCurveName))
	{
		Pin->bHidden = (GetNode()->AlphaInputType != EAnimAlphaInputType::Curve);

		if (!Pin->bHidden)
		{
			Pin->PinFriendlyName = GetNode()->AlphaScaleBiasClamp.GetFriendlyName(Pin->PinFriendlyName);
		}
	}
}

void UAnimGraphNode_AdditiveBlendSpace::PreloadRequiredAssets()
{
	PreloadObject(GetBlendSpace());

	Super::PreloadRequiredAssets();
}

void UAnimGraphNode_AdditiveBlendSpace::PostProcessPinName(const UEdGraphPin* Pin, FString& DisplayName) const
{
	//if (Pin->Direction == EGPD_Input)
	//{
	//	UBlendSpaceBase* BlendSpace = GetBlendSpace();

	//	if (BlendSpace != NULL)
	//	{
	//		if (Pin->PinName == TEXT("X"))
	//		{
	//			DisplayName = BlendSpace->GetBlendParameter(0).DisplayName;
	//		}
	//		else if (Pin->PinName == TEXT("Y"))
	//		{
	//			DisplayName = BlendSpace->GetBlendParameter(1).DisplayName;
	//		}
	//		else if (Pin->PinName == TEXT("Z"))
	//		{
	//			DisplayName = BlendSpace->GetBlendParameter(2).DisplayName;
	//		}
	//	}
	//}

	Super::PostProcessPinName(Pin, DisplayName);
}

void UAnimGraphNode_AdditiveBlendSpace::SetAnimationAsset(UAnimationAsset* Asset)
{
	if (UBlendSpace * BlendSpace = Cast<UBlendSpace>(Asset))
	{
		GetNode()->SetBlendSpace(BlendSpace);
	}
}

bool UAnimGraphNode_AdditiveBlendSpace::IsAimOffsetBlendSpace(const UClass* BlendSpaceClass)
{
	return  BlendSpaceClass->IsChildOf(UAimOffsetBlendSpace::StaticClass()) ||
		BlendSpaceClass->IsChildOf(UAimOffsetBlendSpace1D::StaticClass());
}

#undef LOCTEXT_NAMESPACE