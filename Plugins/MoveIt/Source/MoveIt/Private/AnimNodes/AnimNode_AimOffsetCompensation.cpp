// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.

#include "AnimNodes/AnimNode_AimOffsetCompensation.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/BlendProfile.h"
#include "MIAnimInstanceProxy.h"
#include "MIAnimInstance.h"
#include "Kismet/KismetMathLibrary.h"
#include "AnimationRuntime.h"

DEFINE_LOG_CATEGORY_STATIC(LogAimOffsetComp, Log, All);


void FAnimNode_AimOffsetCompensation::Initialize_AnyThread(const FAnimationInitializeContext& Context)
{
	FAnimNode_LocalSkeletalControlBase::Initialize_AnyThread(Context);

	AimOffsetTurn = 0.f;
}

void FAnimNode_AimOffsetCompensation::PreUpdate(const UAnimInstance* InAnimInstance)
{
	AimOffsetTurn = 0.f;
	if (const UMIAnimInstance* MIAnimInstance = Cast<UMIAnimInstance>(InAnimInstance))
	{
		AimOffsetTurn = MIAnimInstance->AimOffsetTurn;
	}
}

void FAnimNode_AimOffsetCompensation::EvaluateLocalSkeletalControl_AnyThread(FPoseContext& Output, float ActualBlendWeight)
{
	const FBoneContainer& BoneContainer = Output.AnimInstanceProxy->GetRequiredBones();

	FTransform ComponentTransform = Output.AnimInstanceProxy->GetComponentTransform();

	const float AimAmount = (1.f / BoneCache.Num());

	// Support Non-UE4 characters (may also be needed for quadrupeds)
	auto OnGetAimAdjust = [this]() -> FRotator
	{
		switch (AimAxis)
		{
		case EAxis::X:
			return FRotator(0.f, AimOffsetTurn, 0.f);
		case EAxis::Y:
			return FRotator(AimOffsetTurn, 0.f, 0.f);
		case EAxis::Z:
			return FRotator(0.f, 0.f, AimOffsetTurn);
		case EAxis::None:
		default:
			return FRotator::ZeroRotator;
		}
	};

	const FRotator AimAdjust = OnGetAimAdjust();

	// Abort if no changes to be made
	if (FMath::IsNearlyZero(ActualBlendWeight * AimAmount))
	{
		return;
	}

	for (const FMIAimOffsetBone& Bone : BoneCache)
	{
		FTransform BoneTM = ConvertLocalToComponent(Output, Bone.Index);
		const FQuat BodyMeshSpaceQuat = BoneTM.GetRotation().Inverse() * AimAdjust.Quaternion() * BoneTM.GetRotation();
		BoneTM.ConcatenateRotation(BodyMeshSpaceQuat);
		BoneTM.NormalizeRotation();
		ApplyComponentToLocal(Output, BoneTM, Bone.Index, ActualBlendWeight * AimAmount);
	}
}

bool FAnimNode_AimOffsetCompensation::IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones)
{
	for (FMIAimOffsetBone& Bone : BoneCache)
	{
		if (!Bone.Bone.IsValidToEvaluate(RequiredBones)) { return false; }
	}

	return BoneCache.Num() > 0 && AimAxis != EAxis::None;
}

void FAnimNode_AimOffsetCompensation::InitializeBoneParentCache(const FBoneContainer& BoneContainer)
{
	for (FMIAimOffsetBone& Bone : BoneCache)
	{
		InitializeBoneParents(Bone.Index, BoneContainer);
	}
}

void FAnimNode_AimOffsetCompensation::InitializeBoneReferences(const FBoneContainer& RequiredBones)
{
	BoneCache.Reset();

	for (FBoneReference& Bone : Bones)
	{
		Bone.Initialize(RequiredBones);

		BoneCache.Add(FMIAimOffsetBone(Bone, Bone.GetCompactPoseIndex(RequiredBones)));
	}
}
