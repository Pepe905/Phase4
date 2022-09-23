// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AnimGraphNode_LocalSkeletalControlBase.h"
#include "AnimNodes/AnimNode_LandingCompression.h"
#include "AnimGraphNode_LandingCompression.generated.h"

/**
 * 
 */
UCLASS()
class MOVEITEDITOR_API UAnimGraphNode_LandingCompression : public UAnimGraphNode_LocalSkeletalControlBase
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, Category=Settings)
	FAnimNode_LandingCompression Node;

public:
	//~ Begin UEdGraphNode Interface
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FText GetTooltipText() const override;
	virtual FString GetNodeCategory() const override;
	//~ End UEdGraphNode Interface

	// UAnimGraphNode_Base interface
	virtual void CustomizePinData(UEdGraphPin* Pin, FName SourcePropertyName, int32 ArrayIndex) const override;
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
	// End of UAnimGraphNode_Base interface

protected:
	//~ Begin UAnimGraphNode_SkeletalControlBase Interface
	virtual FText GetControllerDescription() const override;
	virtual const FAnimNode_LocalSkeletalControlBase* GetNode() const { return &Node; }
	//~ End UAnimGraphNode_SkeletalControlBase Interface

private:
	/** Constructing FText strings can be costly, so we cache the node's title */
	FNodeTitleTextTable CachedNodeTitles;
};
