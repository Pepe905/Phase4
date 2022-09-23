// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.

#include "AnimNodes/AnimNode_TraversalSpring.h"
#include "Animation/AnimInstanceProxy.h"
#include "MICharacter.h"
#include "MICharacterMovementComponent.h"
#include "GameFramework/WorldSettings.h"


void FAnimNode_TraversalSpring::Initialize_AnyThread(const FAnimationInitializeContext& Context)
{
	FAnimNode_LocalSkeletalControlBase::Initialize_AnyThread(Context);

	RemainingTime = 0.f;

	const USkeletalMeshComponent* const SkelComp = Context.AnimInstanceProxy->GetSkelMeshComponent();
	Character = (SkelComp && SkelComp->GetOwner()) ? Cast<AMICharacter>(SkelComp->GetOwner()) : nullptr;
}

void FAnimNode_TraversalSpring::PreUpdate(const UAnimInstance* InAnimInstance)
{
	// Copied from FAnimNode_SpringBone::PreUpdate

	const USkeletalMeshComponent* SkelComp = InAnimInstance->GetSkelMeshComponent();
	const UWorld* World = SkelComp->GetWorld();
	check(World->GetWorldSettings());
	TimeDilation = World->GetWorldSettings()->GetEffectiveTimeDilation();

	OwnerVelocity = (SkelComp->GetOwner()) ? SkelComp->GetOwner()->GetVelocity() : FVector::ZeroVector;
}

void FAnimNode_TraversalSpring::UpdateInternal(const FAnimationUpdateContext& Context)
{
	RemainingTime += Context.GetDeltaTime();

	// Fixed step simulation at 120hz
	FixedTimeStep = (1.f / 120.f) * TimeDilation;
}

void FAnimNode_TraversalSpring::EvaluateLocalSkeletalControl_AnyThread(FPoseContext& Output, float ActualBlendWeight)
{
	if (!Character || !Character->GetMICharacterMovement())
	{
		return;
	}

	// Cache vars for use
	const UWorld* const World = Output.AnimInstanceProxy->GetAnimInstanceObject()->GetWorld();

	const USkeletalMeshComponent* SkelComp = Output.AnimInstanceProxy->GetSkelMeshComponent();
	const FTransform& MeshTM = Output.AnimInstanceProxy->GetComponentTransform();

	const FBoneContainer& BoneContainer = Output.AnimInstanceProxy->GetRequiredBones();

	FTransform PelvisTM = ConvertLocalToComponent(Output, BoneIndex);

	const FVector TraversalVelocity = Character->GetMICharacterMovement()->GetTraversalVelocity();
	const float DisplacementMultiplier = TraversalVelocity.Z > 0.f ? SpringUpwardStrength : SpringDownwardStrength;

	if (SpringUpwardStrength != 0.f)
	{
		// Reset velocity values each frame
		BoneLocation = PelvisTM.GetLocation();
		BoneVelocity = FVector::ZeroVector;

		if (!FMath::IsNearlyZero(FixedTimeStep, KINDA_SMALL_NUMBER))
		{
			while (RemainingTime > FixedTimeStep)
			{
				// Based largely on FAnimNode_SpringBone

				// Calculate error vector.
				const FVector Error = FVector::UpVector * TraversalVelocity.Z;
				const FVector DampingForce = SpringDamping * BoneVelocity;
				const FVector SpringForce = SpringStiffness * Error;

				// Calculate force based on error and vel
				const FVector Acceleration = SpringForce - DampingForce;

				// Integrate velocity
				// Make sure damping with variable frame rate actually dampens velocity. Otherwise Spring will go nuts.
				float const CutOffDampingValue = 1.f / FixedTimeStep;
				if (SpringDamping > CutOffDampingValue)
				{
					const float SafetyScale = CutOffDampingValue / SpringDamping;
					BoneVelocity += SafetyScale * (Acceleration * FixedTimeStep);
				}
				else
				{
					BoneVelocity += (Acceleration * FixedTimeStep);
				}

				// Integrate position
				const FVector OldBoneLocation = BoneLocation;
				const FVector DeltaMove = (BoneVelocity * FixedTimeStep);
				BoneLocation += DeltaMove;

				// Update velocity to reflect post processing done to bone location.
				BoneVelocity = (BoneLocation - OldBoneLocation) / FixedTimeStep;
				BoneVelocity *= DisplacementMultiplier;

				check(!BoneLocation.ContainsNaN());
				check(!BoneVelocity.ContainsNaN());

				RemainingTime -= FixedTimeStep;
			}
		}

		// Compute pelvis location
		const FVector PelvisOriginLocation = PelvisTM.GetLocation();
		const FVector PelvisNewLocation = PelvisOriginLocation + BoneVelocity;
		const FVector PelvisTargetLocation = PelvisOriginLocation + (BoneVelocity);

		// Apply pelvis transform
		PelvisTM.SetTranslation(PelvisTargetLocation);
		ApplyComponentToLocal(Output, PelvisTM, BoneIndex, ActualBlendWeight);
	}
}

bool FAnimNode_TraversalSpring::IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones)
{
	return Bone.IsValidToEvaluate(RequiredBones);
}

void FAnimNode_TraversalSpring::InitializeBoneParentCache(const FBoneContainer& BoneContainer)
{
	InitializeBoneParents(Bone, BoneContainer);
}

void FAnimNode_TraversalSpring::InitializeBoneReferences(const FBoneContainer& RequiredBones)
{
	Bone.Initialize(RequiredBones);
	BoneIndex = Bone.GetCompactPoseIndex(RequiredBones);
}
