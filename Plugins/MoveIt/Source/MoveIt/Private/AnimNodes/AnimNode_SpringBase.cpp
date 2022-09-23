// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.

#include "AnimNodes/AnimNode_SpringBase.h"
#include "Animation/AnimInstanceProxy.h"
#include "GameFramework/WorldSettings.h"


void FAnimNode_SpringBase::Initialize_AnyThread(const FAnimationInitializeContext& Context)
{
	FAnimNode_LocalSkeletalControlBase::Initialize_AnyThread(Context);

	RemainingTime = 0.f;
	Velocity = FVector::ZeroVector;
}

void FAnimNode_SpringBase::PreUpdate(const UAnimInstance* InAnimInstance)
{
	const USkeletalMeshComponent* SkelComp = InAnimInstance->GetSkelMeshComponent();
	const UWorld* World = SkelComp->GetWorld();
	check(World->GetWorldSettings());
	TimeDilation = World->GetWorldSettings()->GetEffectiveTimeDilation();

	AActor* SkelOwner = SkelComp->GetOwner();
	if (SkelComp && SkelComp->GetAttachParent() != NULL && (SkelOwner == NULL))
	{
		SkelOwner = SkelComp->GetAttachParent()->GetOwner();
		OwnerVelocity = SkelOwner->GetVelocity();
	}
	else
	{
		OwnerVelocity = FVector::ZeroVector;
	}
}

void FAnimNode_SpringBase::UpdateInternal(const FAnimationUpdateContext& Context)
{
	FAnimNode_LocalSkeletalControlBase::UpdateInternal(Context);

	RemainingTime += Context.GetDeltaTime();

	// Fixed step simulation at 120hz
	FixedTimeStep = (1.f / SimulationFrequency) * TimeDilation;
}

FORCEINLINE void CopyToVectorByFlags(FVector& DestVec, const FVector& SrcVec, bool bX, bool bY, bool bZ)
{
	if (bX)
	{
		DestVec.X = SrcVec.X;
	}
	if (bY)
	{
		DestVec.Y = SrcVec.Y;
	}
	if (bZ)
	{
		DestVec.Z = SrcVec.Z;
	}
}

void FAnimNode_SpringBase::EvaluateLocalSkeletalControl_AnyThread(FPoseContext& Output, float ActualBlendWeight)
{
	const bool bNoOffset = !bTranslateX && !bTranslateY && !bTranslateZ;
	if (bNoOffset)
	{
		//return;
	}

	// Location of our bone in world space
	const FBoneContainer& BoneContainer = Output.AnimInstanceProxy->GetRequiredBones();

	const FCompactPoseBoneIndex SpringBoneIndex = SpringBone.GetCompactPoseIndex(BoneContainer);
	FTransform SpaceBase = ConvertLocalToComponent(Output, SpringBoneIndex);
	FTransform BoneTransformInWorldSpace = (bSimulateInComponentSpace) ? SpaceBase : SpaceBase * Output.AnimInstanceProxy->GetComponentTransform();

	FVector const TargetPos = BoneTransformInWorldSpace.GetLocation();

	// Init values first time
	if (RemainingTime == 0.0f)
	{
		BoneLocation = TargetPos;
		BoneVelocity = FVector::ZeroVector;
	}

	if (!FMath::IsNearlyZero(FixedTimeStep, KINDA_SMALL_NUMBER))
	{
		while (RemainingTime > FixedTimeStep)
		{
			// Update location of our base by how much our base moved this frame.
			FVector const BaseTranslation = ((bSimulateInComponentSpace ? Velocity : OwnerVelocity) * FixedTimeStep);
			BoneLocation += BaseTranslation;

			// Reinit values if outside reset threshold
			if (((TargetPos - BoneLocation).SizeSquared() > (ErrorResetThresh * ErrorResetThresh)))
			{
				BoneLocation = TargetPos;
				BoneVelocity = FVector::ZeroVector;
			}

			//const bool bUp = TargetPos.Z > BoneLocation.Z;

			// Calculate error vector.
			FVector const Error = (TargetPos - BoneLocation);
			FVector const DampingForce = SpringDamping * BoneVelocity;
			FVector const SpringForce = SpringStiffness * Error;

			// Calculate force based on error and vel
			FVector const Acceleration = (SpringForce - DampingForce) / SpringStrength;

			// Integrate velocity
			// Make sure damping with variable frame rate actually dampens velocity. Otherwise Spring will go nuts.
			float const CutOffDampingValue = 1.f / FixedTimeStep;
			if (SpringDamping > CutOffDampingValue)
			{
				float const SafetyScale = CutOffDampingValue / SpringDamping;
				BoneVelocity += SafetyScale * (Acceleration * FixedTimeStep);
			}
			else
			{
				BoneVelocity += (Acceleration * FixedTimeStep);
			}

			// Clamp velocity to something sane (|dX/dt| <= ErrorResetThresh)
			float const BoneVelocityMagnitude = BoneVelocity.Size();
			if (BoneVelocityMagnitude * FixedTimeStep > ErrorResetThresh)
			{
				BoneVelocity *= (ErrorResetThresh / (BoneVelocityMagnitude * FixedTimeStep));
			}

			// Integrate position
			FVector const OldBoneLocation = BoneLocation;
			FVector const DeltaMove = (BoneVelocity * FixedTimeStep);
			BoneLocation += DeltaMove;

			// Filter out spring translation based on our filter properties
			CopyToVectorByFlags(BoneLocation, TargetPos, !bTranslateX, !bTranslateY, !bTranslateZ);

			// Custom code used to limit translation on specific axis
			LimitDisplacement(BoneLocation.X, TargetPos.X, LimitMinTranslation.bX, LimitMaxTranslation.bX, MinTranslationLimits.X, MaxTranslationLimits.X);
			LimitDisplacement(BoneLocation.Y, TargetPos.Y, LimitMinTranslation.bY, LimitMaxTranslation.bY, MinTranslationLimits.Y, MaxTranslationLimits.Y);
			LimitDisplacement(BoneLocation.Z, TargetPos.Z, LimitMinTranslation.bZ, LimitMaxTranslation.bZ, MinTranslationLimits.Z, MaxTranslationLimits.Z);

			// If desired, limit error
			if (bLimitDisplacement)
			{
				FVector CurrentDisp = BoneLocation - TargetPos;
				// Too far away - project back onto sphere around target.
				if (CurrentDisp.SizeSquared() > FMath::Square(MaxDisplacement))
				{
					FVector DispDir = CurrentDisp.GetSafeNormal();
					BoneLocation = TargetPos + (MaxDisplacement * DispDir);
				}
			}

			// Update velocity to reflect post processing done to bone location.
			BoneVelocity = (BoneLocation - OldBoneLocation) / FixedTimeStep;

			check(!BoneLocation.ContainsNaN());
			check(!BoneVelocity.ContainsNaN());

			RemainingTime -= FixedTimeStep;
		}
		LocalBoneTransform = Output.AnimInstanceProxy->GetComponentTransform().InverseTransformPosition(BoneLocation);
	}
	else
	{
		BoneLocation = Output.AnimInstanceProxy->GetComponentTransform().TransformPosition(LocalBoneTransform);
	}
	// Now convert back into component space and output - rotation is unchanged.
	FTransform OutBoneTM = SpaceBase;
	if (bSimulateInComponentSpace)
	{
		OutBoneTM.SetLocation(LocalBoneTransform);
		OutBoneTM *= Output.AnimInstanceProxy->GetComponentTransform();
		OutBoneTM.SetRotation(SpaceBase.GetRotation());
	}
	else
	{
		OutBoneTM.SetLocation(LocalBoneTransform);
	}

	const bool bUseRotation = bRotateX || bRotateY || bRotateZ;
	if (bUseRotation)
	{
		FCompactPoseBoneIndex ParentBoneIndex = BoneContainer.GetParentBoneIndex(SpringBoneIndex);
		const FTransform& ParentSpaceBase = ConvertLocalToComponent(Output, ParentBoneIndex);

		FVector ParentToTarget = (TargetPos - ParentSpaceBase.GetLocation()).GetSafeNormal();
		FVector ParentToCurrent = (BoneLocation - ParentSpaceBase.GetLocation()).GetSafeNormal();

		FQuat AdditionalRotation = FQuat::FindBetweenNormals(ParentToTarget, ParentToCurrent);

		// Filter rotation based on our filter properties
		FVector EularRot = AdditionalRotation.Euler();
		CopyToVectorByFlags(EularRot, FVector::ZeroVector, !bRotateX, !bRotateY, !bRotateZ);

		OutBoneTM.SetRotation(FQuat::MakeFromEuler(EularRot) * OutBoneTM.GetRotation());
	}

	OutBoneTM.BlendWith(SpaceBase, 1.f - ActualBlendWeight);
	OutBoneTM.SetScale3D(FVector::OneVector);  // This solves any issues with scaled meshes

	ConvertComponentToLocal(OutBoneTM, SpringBoneIndex);
}

bool FAnimNode_SpringBase::IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones)
{
	return SpringBone.IsValidToEvaluate(RequiredBones);
}

void FAnimNode_SpringBase::InitializeBoneParentCache(const FBoneContainer& BoneContainer)
{
	InitializeBoneParents(SpringBone, BoneContainer);
}

void FAnimNode_SpringBase::InitializeBoneReferences(const FBoneContainer& RequiredBones)
{
	SpringBone.Initialize(RequiredBones);
}
