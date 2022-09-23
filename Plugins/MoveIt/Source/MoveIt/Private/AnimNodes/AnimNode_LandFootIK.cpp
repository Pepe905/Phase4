// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.

#include "AnimNodes/AnimNode_LandFootIK.h"
#include "Animation/AnimInstanceProxy.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStaticsTypes.h"
#include "Kismet/GameplayStatics.h"
#include "MITypes.h"
#include "Kismet/KismetMathLibrary.h"


void FAnimNode_LandFootIK::Initialize_AnyThread(const FAnimationInitializeContext& Context)
{
	FAnimNode_LocalSkeletalControlBase::Initialize_AnyThread(Context);

	for (FMIFoot_LandFootIK& Foot : Feet)
	{
		Foot.bReinitTarget = true;
	}

	const USkeletalMeshComponent* const SkelComp = Context.AnimInstanceProxy->GetSkelMeshComponent();
	Character = (SkelComp && SkelComp->GetOwner()) ? Cast<ACharacter>(SkelComp->GetOwner()) : nullptr;
}

void FAnimNode_LandFootIK::PreUpdate(const UAnimInstance* InAnimInstance)
{
	const USkeletalMeshComponent* const SkelComp = InAnimInstance->GetSkelMeshComponent();

	if (!SkelComp || !Character || !Character->GetCharacterMovement() || !Character->GetWorld())
	{
		return;
	}

	if (!Character->GetWorld()->IsGameWorld() && !bWorkOutsidePIE) { return; }

	bool bCanLand = Character->GetCharacterMovement()->IsFalling() && Character->GetVelocity().Z < JumpStartZVelocity;

	// Predict the landing location
	bCanLand &= PredictedLanding.HitResult.bBlockingHit;

	FHitResult& Hit = PredictedLanding.HitResult;

	// Landing location is walkable
	bCanLand &= Character->GetCharacterMovement()->IsWalkable(Hit);

	// Ensure we are close enough to the ground
	const float DistToGround = Character->GetActorLocation().Z - Hit.ImpactPoint.Z;
	if (bCanLand)
	{
		bCanLand &= DistToGround <= StartMinDistFromGround;
	}

	if (bCanLand && !bAlphaBoolEnabled)
	{
		AlphaBoolBlend.AlphaBlend.SetAlpha(1.f);

		// Just started falling
		for (FMIFoot_LandFootIK& Foot : Feet)
		{
			Foot.bReinitTarget = true;
		}
	}

	bAlphaBoolEnabled = bCanLand;

	OwnerVelocity = Character ? Character->GetVelocity() : FVector::ZeroVector;
}

void FAnimNode_LandFootIK::UpdateInternal(const FAnimationUpdateContext& Context)
{
	DeltaTime = Context.GetDeltaTime();
}

void FAnimNode_LandFootIK::EvaluateLocalSkeletalControl_AnyThread(FPoseContext& Output, float ActualBlendWeight)
{
	const USkeletalMeshComponent* SkelComp = Output.AnimInstanceProxy->GetSkelMeshComponent();
	if (!SkelComp || !SkelComp->GetOwner() || !Character)
	{
		return;
	}

	// Cache vars for use
	const FBoneContainer& BoneContainer = Output.AnimInstanceProxy->GetRequiredBones();
	const FTransform& MeshTM = Output.AnimInstanceProxy->GetComponentTransform();
	UWorld* const World = Output.AnimInstanceProxy->GetAnimInstanceObject()->GetWorld();

	// Not falling, do nothing
	bool bCanLand = true;

	// Update foot transforms
	for (FMIFoot_LandFootIK& Foot : Feet)
	{
		Foot.CachedTM = ConvertLocalToComponent(Output, Foot.IKBoneIndex);
		Foot.CachedWorldTM = Foot.CachedTM * MeshTM;
		Foot.LandTargetTM = Foot.CachedWorldTM;

		Foot.FootComponentOffset = Foot.CachedTM.GetTranslation();
	}

	FHitResult& Hit = PredictedLanding.HitResult;

	if (bCanLand)
	{
		// Get rotation to predict location
		const FRotator DeltaRot = (Hit.ImpactPoint - MeshTM.GetLocation()).Rotation();

		//DrawDebugDirectionalArrow(World, MeshTM.GetLocation(), MeshTM.GetLocation() + DeltaRot.Vector() * 100.f, 40.f, FColor::White, false, -1.f, 0, 1.f);

		FRotator LegRot = MeshTM.InverseTransformRotation(DeltaRot.Quaternion()).Rotator() - FRotator(0.f, 90.f, 0.f);
		LegRot.Normalize();

		const float Mag = (Hit.ImpactPoint - MeshTM.GetLocation()).Size2D();
		//UE_LOG(LogTemp, Log, TEXT("rt %f"), Mag);
	}

	const FTransform& PelvisTM = ConvertLocalToComponent(Output, PelvisIndex);
	for (FMIFoot_LandFootIK& Foot : Feet)
	{
		if (bCanLand)
		{
			// Land target in world space
			Foot.LandTargetTM = FTransform(Foot.CachedWorldTM.Rotator(), Hit.ImpactPoint, FVector::OneVector);

			// Convert to component space
			Foot.LandTargetTM = Foot.LandTargetTM.GetRelativeTransform(MeshTM);
			Foot.LandTargetTM.SetTranslation(Foot.LandTargetTM.GetTranslation() + Foot.FootComponentOffset);
		}
		else
		{
			Foot.LandTargetTM = Foot.CachedTM;
		}

		if (Foot.bReinitTarget)
		{
			Foot.LandCurrentTM = Foot.CachedTM;
		}

		const FVector Translation = FMath::VInterpTo(Foot.LandCurrentTM.GetLocation(), Foot.LandTargetTM.GetLocation(), DeltaTime, Foot.InterpRate);
		Foot.LandCurrentTM.SetLocation(Translation);

		ApplyComponentToLocal(Output, Foot.LandCurrentTM, Foot.IKBoneIndex, Foot.ReachPct * ActualBlendWeight);
	}
}


bool FAnimNode_LandFootIK::IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones)
{
	if (!Pelvis.IsValidToEvaluate(RequiredBones)) { return false; }

	for (FMIFoot_LandFootIK& Foot : Feet)
	{
		if (!Foot.Bone.IsValidToEvaluate(RequiredBones)) { return false; }
		if (!Foot.IKBone.IsValidToEvaluate(RequiredBones)) { return false; }
	}
	return Feet.Num() > 0;
}

void FAnimNode_LandFootIK::InitializeBoneParentCache(const FBoneContainer& BoneContainer)
{
	InitializeBoneParents(Pelvis, BoneContainer);

	for (FMIFoot_LandFootIK& Foot : Feet)
	{
		InitializeBoneParents(Foot.IKBone, BoneContainer);
	}
}

void FAnimNode_LandFootIK::InitializeBoneReferences(const FBoneContainer& RequiredBones)
{
	Pelvis.Initialize(RequiredBones);
	PelvisIndex = Pelvis.GetCompactPoseIndex(RequiredBones);

	Feet.Reset();
	for (FMIFoot_LandFootIK& Foot : LeftFeet)
	{
		Foot.bIsLeftFoot = true;
		Feet.Add(Foot);
	}
	for (FMIFoot_LandFootIK& Foot : RightFeet)
	{
		Feet.Add(Foot);
	}

	for (FMIFoot_LandFootIK& Foot : Feet)
	{
		Foot.Bone.Initialize(RequiredBones);
		Foot.IKBone.Initialize(RequiredBones);

		Foot.BoneIndex = Foot.Bone.GetCompactPoseIndex(RequiredBones);
		Foot.IKBoneIndex = Foot.IKBone.GetCompactPoseIndex(RequiredBones);

		//Foot.KneeBoneIndex = RequiredBones.GetParentBoneIndex(Foot.BoneIndex);
		//Foot.ThighBoneIndex = RequiredBones.GetParentBoneIndex(Foot.KneeBoneIndex);
	}
}
