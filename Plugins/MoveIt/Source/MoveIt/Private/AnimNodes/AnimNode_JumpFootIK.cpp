// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.

#include "AnimNodes/AnimNode_JumpFootIK.h"
#include "Animation/AnimInstanceProxy.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"


void FAnimNode_JumpFootIK::Initialize_AnyThread(const FAnimationInitializeContext& Context)
{
	FAnimNode_LocalSkeletalControlBase::Initialize_AnyThread(Context);

	const USkeletalMeshComponent* const SkelComp = Context.AnimInstanceProxy->GetSkelMeshComponent();
	Character = (SkelComp && SkelComp->GetOwner()) ? Cast<ACharacter>(SkelComp->GetOwner()) : nullptr;
}

void FAnimNode_JumpFootIK::PreUpdate(const UAnimInstance* InAnimInstance)
{
	const USkeletalMeshComponent* const SkelComp = InAnimInstance->GetSkelMeshComponent();

	OwnerVelocity = (SkelComp && SkelComp->GetOwner()) ? SkelComp->GetOwner()->GetVelocity() : FVector::ZeroVector;
}

void FAnimNode_JumpFootIK::UpdateInternal(const FAnimationUpdateContext& Context)
{
	DeltaTime = Context.GetDeltaTime();
}

void FAnimNode_JumpFootIK::EvaluateLocalSkeletalControl_AnyThread(FPoseContext& Output, float ActualBlendWeight)
{
	const USkeletalMeshComponent* SkelComp = Output.AnimInstanceProxy->GetSkelMeshComponent();
	if (!SkelComp || !SkelComp->GetOwner() || !Character || !Character->GetCharacterMovement())
	{
		return;
	}

	// Not falling, do nothing
	if (!Character->GetCharacterMovement()->IsFalling())
	{
		bWasFalling = false;

		// If no foot needs processing, just return
		bool bProcess = false;
		for (FMIJumpFootIK_Foot& Foot : Feet)
		{
			if (Foot.BlendAlpha != 0.f)
			{
				bProcess = true;
				break;
			}
		}

		if (!bProcess)
		{
			return;
		}
	}

	// Compute current relevant movement state
	if (Character->GetCharacterMovement()->IsMovingOnGround() || Character->GetCharacterMovement()->IsFlying())
	{
		State = EMIJumpState::JS_Ground;
	}
	else if (Character->GetCharacterMovement()->IsFalling())
	{
		if (OwnerVelocity.Z > 0.f)
		{
			State = EMIJumpState::JS_Jump;
		}
		else
		{
			State = EMIJumpState::JS_Fall;
		}
	}
	else if (Character->GetCharacterMovement()->IsSwimming())
	{
		State = EMIJumpState::JS_Swim;
	}
	else if (Character->GetCharacterMovement()->IsFlying())
	{
		State = EMIJumpState::JS_Fly;
	}
	else
	{
		State = EMIJumpState::JS_None;
	}

	// Cache vars for use
	const UWorld* const World = Output.AnimInstanceProxy->GetAnimInstanceObject()->GetWorld();
	const FBoneContainer& BoneContainer = Output.AnimInstanceProxy->GetRequiredBones();
	const FTransform& MeshTM = Output.AnimInstanceProxy->GetComponentTransform();
	const TArray<AActor*> TraceIgnore{ Character };

	// Update foot transforms
	for (FMIJumpFootIK_Foot& Foot : Feet)
	{
		Foot.CachedTM = ConvertLocalToComponent(Output, Foot.IKBoneIndex);
		Foot.CachedWorldTM = Foot.CachedTM * MeshTM;
	}

	// Just started falling
	if (Character->GetCharacterMovement()->IsFalling() && !bWasFalling)
	{
		// Find which foot is behind us (to be jumped off)
		JumpingFoot = nullptr;
		float FootDistance = 0.f;
		for (FMIJumpFootIK_Foot& Foot : Feet)
		{
			//DrawDebugPoint(World, Foot.CachedWorldTM.GetLocation(), 24.f, FColor::Red, false, -1.f, 10);

			// If we are moving, then the farthest foot from the velocity trajectory is the jumping foot
			// if we are standing still, use the back foot (relative to facing vector)

			const FVector FacingVector = (!FMath::IsNearlyZero(OwnerVelocity.Size2D(), KINDA_SMALL_NUMBER)) ? OwnerVelocity.GetSafeNormal2D() : MeshTM.GetScaledAxis(EAxis::Y);
			const FVector FootTrajectory = MeshTM.GetLocation() + (FacingVector * 200.f);

			const float FootDist = (FootTrajectory - Foot.CachedWorldTM.GetLocation()).Size2D();
			const bool bBetterFoot = (bUseRearFoot) ? (FootDist > FootDistance) : (FootDist < FootDistance);
			if (!JumpingFoot || bBetterFoot)
			{
				JumpingFoot = &Foot;
				FootDistance = FootDist;
			}

			//DrawDebugDirectionalArrow(World, MeshTM.GetLocation(), MeshTM.GetLocation() + (FacingVector * 200.f), 100.f, FColor::Green, false, -1.f, 0, 2.f);
		}

		if (JumpingFoot)
		{
			bWasFalling = true;

			JumpingFoot->JumpStartTM = JumpingFoot->CachedTM;
			JumpingFoot->BlendAlpha = 1.f;
		}
	}

	for (FMIJumpFootIK_Foot& Foot : Feet)
	{
		if (Foot.BlendAlpha == 0.f || Foot.JumpStartTM.GetLocation().IsZero())
		{
			continue;
		}

		float BlendOutRate = Foot.DefaultInterpOutRate;
		switch (State)
		{
		case EMIJumpState::JS_Ground:
			BlendOutRate = Foot.GroundInterpOutRate;
			break;
		case EMIJumpState::JS_Jump:
			BlendOutRate = Foot.JumpInterpOutRate;
			break;
		case EMIJumpState::JS_Fall:
			BlendOutRate = Foot.FallInterpOutRate;
			break;
		case EMIJumpState::JS_Fly:
			BlendOutRate = Foot.FlyInterpOutRate;
			break;
		case EMIJumpState::JS_Swim:
			BlendOutRate = Foot.SwimInterpOutRate;
			break;
		case EMIJumpState::JS_None:
		default:
			BlendOutRate = Foot.DefaultInterpOutRate;
			break;
		}
		Foot.BlendAlpha = FMath::FInterpTo(Foot.BlendAlpha, 0.f, DeltaTime, BlendOutRate);
		if (FMath::IsNearlyZero(Foot.BlendAlpha, ZERO_ANIMWEIGHT_THRESH))
		{
			Foot.BlendAlpha = 0.f;
		}

		ApplyComponentToLocal(Output, Foot.JumpStartTM, Foot.IKBoneIndex, Foot.BlendAlpha * ActualBlendWeight);
	}
}

bool FAnimNode_JumpFootIK::IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones)
{
	for (FMIJumpFootIK_Foot& Foot : Feet)
	{
		if (!Foot.Foot.IsValidToEvaluate(RequiredBones)) { return false; }
		if (!Foot.IKFoot.IsValidToEvaluate(RequiredBones)) { return false; }

		for (const FCompactPoseBoneIndex& BoneIndex : Foot.FKBoneIndices)
		{
			if (BoneIndex == INDEX_NONE) { return false; }
		}
	}
	return Feet.Num() > 0;
}

void FAnimNode_JumpFootIK::InitializeBoneParentCache(const FBoneContainer& BoneContainer)
{
	for (FMIJumpFootIK_Foot& Foot : Feet)
	{
		InitializeBoneParents(Foot.IKFoot, BoneContainer);

		for (const FCompactPoseBoneIndex& BoneIndex : Foot.FKBoneIndices)
		{
			InitializeBoneParents(BoneIndex, BoneContainer);
		}
	}
}

void FAnimNode_JumpFootIK::InitializeBoneReferences(const FBoneContainer& RequiredBones)
{
	Feet.Reset();
	for (FMIJumpFootIK_Foot& Foot : LeftFeet)
	{
		Foot.bIsLeftFoot = true;
		Feet.Add(Foot);
	}
	for (FMIJumpFootIK_Foot& Foot : RightFeet)
	{
		Feet.Add(Foot);
	}

	for (FMIJumpFootIK_Foot& Foot : Feet)
	{
		Foot.Foot.Initialize(RequiredBones);
		Foot.IKFoot.Initialize(RequiredBones);

		Foot.BoneIndex = Foot.Foot.GetCompactPoseIndex(RequiredBones);
		Foot.IKBoneIndex = Foot.IKFoot.GetCompactPoseIndex(RequiredBones);

		// Cache indices for every bone in the leg chain, used to determine leg length later
		Foot.FKBoneIndices.Reset();
		if (Foot.BoneIndex != INDEX_NONE && Foot.IKBoneIndex != INDEX_NONE)
		{
			FCompactPoseBoneIndex BoneIndex = Foot.BoneIndex;

			Foot.FKBoneIndices.Add(BoneIndex);
			FCompactPoseBoneIndex ParentBoneIndex = RequiredBones.GetParentBoneIndex(BoneIndex);

			int32 NumIterations = Foot.NumBonesInLimb;
			while ((NumIterations-- > 0) && (ParentBoneIndex != INDEX_NONE))
			{
				BoneIndex = ParentBoneIndex;
				Foot.FKBoneIndices.Add(BoneIndex);
				ParentBoneIndex = RequiredBones.GetParentBoneIndex(BoneIndex);
			};
		}
	}
}
