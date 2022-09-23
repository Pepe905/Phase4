// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.

#include "AnimGraphNode_FootIK.h"

#define LOCTEXT_NAMESPACE "MoveItEditor_Node"

FText UAnimGraphNode_FootIK::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return GetControllerDescription();
}

FLinearColor UAnimGraphNode_FootIK::GetNodeTitleColor() const
{
	return FLinearColor::Black;
}

FText UAnimGraphNode_FootIK::GetTooltipText() const
{
	return LOCTEXT("AnimGraphNode_FootIK_Tooltip", "Computes and applies advanced IK for each foot, shifts the weight of the pelvis, handles orientation to floor.");
}

FString UAnimGraphNode_FootIK::GetNodeCategory() const
{
	return TEXT("MoveIt!");
}

FText UAnimGraphNode_FootIK::GetControllerDescription() const
{
	return LOCTEXT("FootIK", "Foot IK");
}

void UAnimGraphNode_FootIK::Draw(FPrimitiveDrawInterface* PDI, USkeletalMeshComponent* SkelMeshComp) const
{
	if (bEnableDebugDraw && SkelMeshComp)
	{
		if (FAnimNode_FootIK* ActiveNode = GetActiveInstanceNode<FAnimNode_FootIK>(SkelMeshComp->GetAnimInstance()))
		{
			ActiveNode->ConditionalDebugDraw(PDI, SkelMeshComp);
		}
	}
}

#undef LOCTEXT_NAMESPACE