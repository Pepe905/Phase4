// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AnimGraphNode_SkeletalControlBase.h"
#include "AnimNodes/AnimNode_FootIK.h"
#include "AnimGraphNode_FootIK.generated.h"

struct FAnimNode_FootIK;

UCLASS()
class MOVEITEDITOR_API UAnimGraphNode_FootIK : public UAnimGraphNode_SkeletalControlBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = Settings)
	FAnimNode_FootIK Node;

	/** Enable drawing of the debug information of the node */
	UPROPERTY(EditAnywhere, Category = Debug)
	bool bEnableDebugDraw;

	UAnimGraphNode_FootIK()
	{
		bEnableDebugDraw = true;
	}

	// UEdGraphNode interface
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual FText GetTooltipText() const override;
	virtual FString GetNodeCategory() const override;
	// End of UEdGraphNode interface

protected:
	//~ Begin UAnimGraphNode_SkeletalControlBase Interface
	virtual FText GetControllerDescription() const override;
	virtual const FAnimNode_SkeletalControlBase* GetNode() const override { return &Node; }
	virtual void Draw(FPrimitiveDrawInterface* PDI, USkeletalMeshComponent* SkelMeshComp) const override;
	//~ End UAnimGraphNode_SkeletalControlBase Interface
};