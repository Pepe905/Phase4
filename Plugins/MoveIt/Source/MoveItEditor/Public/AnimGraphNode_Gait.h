// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AnimGraphNode_LocalSkeletalControlBase.h"
#include "AnimNodes/AnimNode_Gait.h"
#include "AnimGraphNode_Gait.generated.h"

UCLASS()
class MOVEITEDITOR_API UAnimGraphNode_Gait : public UAnimGraphNode_LocalSkeletalControlBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = Settings)
	FAnimNode_Gait Node;

	// UEdGraphNode interface
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FText GetTooltipText() const override;
	virtual FString GetNodeCategory() const override;
	// End of UEdGraphNode interface

protected:
	// Returns the short descriptive name of the controller
	virtual FText GetControllerDescription() const override;

	virtual const FAnimNode_LocalSkeletalControlBase* GetNode() const { return &Node; }
};