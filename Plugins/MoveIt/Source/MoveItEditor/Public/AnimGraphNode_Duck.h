// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AnimGraphNode_AdditiveBlendSpace.h"
#include "AnimNodes/AnimNode_Duck.h"
#include "AnimGraphNode_Duck.generated.h"

UCLASS()
class MOVEITEDITOR_API UAnimGraphNode_Duck : public UAnimGraphNode_AdditiveBlendSpace
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = Settings)
	FAnimNode_Duck Node;

	// UEdGraphNode interface
	virtual FText GetControllerDescription() const override;
	virtual FText GetTooltipText() const override;
	// End of UEdGraphNode interface

protected:
	virtual FAnimNode_AdditiveBlendSpace* GetNode() override { return &Node; }
	virtual const FAnimNode_AdditiveBlendSpace* GetNode() const override { return &Node; }
};