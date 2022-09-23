// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AnimGraphNode_LocalSkeletalControlBase.h"
#include "AnimNodes/AnimNode_Strafe.h"
#include "AnimGraphNode_Strafe.generated.h"

struct FAnimNode_Strafe;

UCLASS()
class MOVEITEDITOR_API UAnimGraphNode_Strafe : public UAnimGraphNode_LocalSkeletalControlBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = Settings)
	FAnimNode_Strafe Node;

	// UEdGraphNode interface
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual FText GetTooltipText() const override;
	virtual FString GetNodeCategory() const override;
	// End of UEdGraphNode interface

protected:
	// Returns the short descriptive name of the controller
	virtual FText GetControllerDescription() const override;

	virtual const FAnimNode_LocalSkeletalControlBase* GetNode() const { return &Node; }
};