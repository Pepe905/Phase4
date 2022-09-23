// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.

#include "AnimGraphNode_OffHandWeaponGrip.h"

#define LOCTEXT_NAMESPACE "MoveItEditor_Node"

FText UAnimGraphNode_OffHandWeaponGrip::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return GetControllerDescription();
}

FLinearColor UAnimGraphNode_OffHandWeaponGrip::GetNodeTitleColor() const
{
	return FLinearColor::Black;
}

FText UAnimGraphNode_OffHandWeaponGrip::GetTooltipText() const
{
	return LOCTEXT("AnimGraphNode_OffHandWeaponGrip_Tooltip", "Procedurally places the off-hand on a weapon.");
}

FString UAnimGraphNode_OffHandWeaponGrip::GetNodeCategory() const
{
	return TEXT("MoveIt!");
}

FText UAnimGraphNode_OffHandWeaponGrip::GetControllerDescription() const
{
	return LOCTEXT("OffHandWeaponGrip", "Off-Hand Weapon Grip");
}

#undef LOCTEXT_NAMESPACE