// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AnimGraphNode_LocalSkeletalControlBase.h"
#include "AnimNodes/AnimNode_LookTarget.h"
#include "AnimGraphNode_LookTarget.generated.h"

struct FAnimNode_LookTarget;

UCLASS()
class MOVEITEDITOR_API UAnimGraphNode_LookTarget : public UAnimGraphNode_LocalSkeletalControlBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = Settings)
	FAnimNode_LookTarget Node;

	// UEdGraphNode interface
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual FText GetTooltipText() const override;
	virtual FString GetNodeCategory() const override;
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	// End of UEdGraphNode interface

	// UAnimGraphNode_Base interface
	virtual void CustomizePinData(UEdGraphPin* Pin, FName SourcePropertyName, int32 ArrayIndex) const override;
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
	// End of UAnimGraphNode_Base interface

protected:
	// Returns the short descriptive name of the controller
	virtual FText GetControllerDescription() const override;

	virtual const FAnimNode_LocalSkeletalControlBase* GetNode() const { return &Node; }
};