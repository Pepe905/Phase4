// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.

#include "AnimNodes/AnimNode_OffHandWeaponGrip.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/BlendProfile.h"
#include "Components/SkeletalMeshComponent.h"
#include "MICharacter.h"
#include "MICharacterMovementComponent.h"
#include "AnimNodes/AnimNodes_Shared.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "Components/CapsuleComponent.h"
#include "MIAnimInstance.h"
#include "MIAnimInstanceProxy.h"


DEFINE_LOG_CATEGORY_STATIC(LogWeaponPose, Log, All);

void FAnimNode_OffHandWeaponGrip::Initialize_AnyThread(const FAnimationInitializeContext& Context)
{
	FAnimNode_SkeletalControlBase::Initialize_AnyThread(Context);

	bValidProxy = Context.AnimInstanceProxy->GetAnimInstanceObject() != nullptr && Context.AnimInstanceProxy->GetAnimInstanceObject()->IsA(UMIAnimInstance::StaticClass());
	if (!bValidProxy)
	{
		UE_LOG(LogWeaponPose, Error, TEXT("Animation Instance must inherit from UMIAnimInstance. Unable to adjust left hand transform."));
	}
}

void FAnimNode_OffHandWeaponGrip::EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms)
{
	check(OutBoneTransforms.Num() == 0);

	if (!bValidProxy)
	{
		return;
	}

	// Cache owner
	USkeletalMeshComponent* const Mesh = Output.AnimInstanceProxy->GetSkelMeshComponent();
	if (!Mesh)
	{
		return;
	}

	// Move Offhand
	if (Weapon && Weapon.WeaponMesh->DoesSocketExist(Weapon.OffHandSocketName))
	{
		FTransform OffHandTM = Output.Pose.GetComponentSpaceTransform(OffHandPoseIndex);
		const FQuat HandRotation = OffHandTM.GetRotation();
		FTransform SocketWorldTM = Weapon.WeaponMesh->GetSocketTransform(Weapon.OffHandSocketName);

		OffHandTM = OffHandTM.GetRelativeTransform(SocketWorldTM);

		FVector BonePos;
		FRotator BoneRot;
		Mesh->TransformToBoneSpace(WeaponHandBone.BoneName, SocketWorldTM.GetLocation(), SocketWorldTM.Rotator(), BonePos, BoneRot);

		OffHandTM = FTransform::Identity;
		OffHandTM.SetLocation(BonePos);
		OffHandTM.SetRotation(BoneRot.Quaternion());

		FMIAnimInstanceProxy* const AnimProxy = ((FMIAnimInstanceProxy*)Output.AnimInstanceProxy);
		AnimProxy->OffHandIKTM = OffHandTM;
	}
}

bool FAnimNode_OffHandWeaponGrip::IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones)
{
	return WeaponHandBone.IsValidToEvaluate(RequiredBones) && OffHandIKBone.IsValidToEvaluate(RequiredBones);
}

void FAnimNode_OffHandWeaponGrip::InitializeBoneReferences(const FBoneContainer& RequiredBones)
{
	WeaponHandBone.Initialize(RequiredBones);
	OffHandIKBone.Initialize(RequiredBones);
	OffHandPoseIndex = OffHandIKBone.GetCompactPoseIndex(RequiredBones);
}

